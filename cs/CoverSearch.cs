using System.Numerics;

namespace KMapReducerWeb.Logic.QuineMcCluskeyAlgorithm
{
    /// <summary>
    /// Branch &amp; bound over the cyclic core. Rows are the pending cells and columns the
    /// candidate cubes, both stored as bitsets of <c>ulong</c> words:
    ///   - cover[i]: pending cells covered by cube i
    ///   - covering[j]: cubes that cover pending cell j
    /// Every node is reduced (forced cubes, row and column dominance) before branching
    /// on the most constrained cell.
    /// </summary>
    /// <remarks>
    /// Column dominance discards cubes that only lead to tied solutions, so the search
    /// runs twice: first with it to get the optimum size fast, then without it, bounded by
    /// that size, to enumerate the tied solutions. The second search can be much slower
    /// and only adds alternatives, so it stops after <see cref="TieSearchNodes"/> nodes.
    ///
    /// The bitsets live in flat arrays allocated once: one <see cref="Frame"/> per depth
    /// for the state that must survive the recursion, and shared scratch buffers for the
    /// rows and columns of the node being reduced.
    /// </remarks>
    internal sealed class CoverSearch
    {
        public const int TieSearchNodes = 5000;

        private readonly int _cubeCount;
        private readonly int _cellCount;
        private readonly int _cubeWords;
        private readonly int _cellWords;
        private readonly int[] _dashes;
        private readonly ulong[] _cover;
        private readonly ulong[] _covering;

        private readonly List<Frame> _frames = new();
        private readonly List<int[]> _solutions = new();
        private int _maxSolutions;
        private int _bestLength;
        private bool _columnDominance;
        private long _nodesLeft;

        // Scratch of the node being reduced; overwritten by the next recursion
        private readonly ulong[] _rows;
        private readonly int[] _rowCell;
        private readonly int[] _rowCount;
        private readonly long[] _rowOrderKeys;
        private readonly int[] _rowOrder;
        private readonly ulong[] _columns;
        private readonly int[] _columnCube;
        private readonly int[] _columnCount;
        private readonly ulong[] _droppedCells;
        private readonly ulong[] _droppedCubes;
        private readonly ulong[] _forced;
        private readonly ulong[] _used;

        /// <summary>State of one depth that its children must not overwrite.</summary>
        private sealed class Frame(int cellWords, int cubeWords, int cubeCount)
        {
            public readonly ulong[] Pending = new ulong[cellWords];
            public readonly ulong[] Allowed = new ulong[cubeWords];
            public readonly ulong[] BranchRow = new ulong[cubeWords];
            public readonly int[] BranchCubes = new int[cubeCount];
            public readonly long[] BranchKeys = new long[cubeCount];
        }

        public CoverSearch(IReadOnlyList<Cube> cubes, IReadOnlyList<int> cells, int maxSolutions)
        {
            _cubeCount = cubes.Count;
            _cellCount = cells.Count;
            _cubeWords = Bits.Words(_cubeCount);
            _cellWords = Bits.Words(_cellCount);
            _maxSolutions = maxSolutions;

            _dashes = new int[_cubeCount];
            _cover = new ulong[_cubeCount * _cellWords];
            _covering = new ulong[_cellCount * _cubeWords];
            for (int i = 0; i < _cubeCount; i++)
            {
                _dashes[i] = cubes[i].Dashes;
                for (int j = 0; j < _cellCount; j++)
                {
                    if (cubes[i].Contains(cells[j]))
                    {
                        Bits.Set(Cover(i), j);
                        Bits.Set(Covering(j), i);
                    }
                }
            }

            _rows = new ulong[_cellCount * _cubeWords];
            _rowCell = new int[_cellCount];
            _rowCount = new int[_cellCount];
            _rowOrderKeys = new long[_cellCount];
            _rowOrder = new int[_cellCount];
            _columns = new ulong[_cubeCount * _cellWords];
            _columnCube = new int[_cubeCount];
            _columnCount = new int[_cubeCount];
            _droppedCells = new ulong[_cellWords];
            _droppedCubes = new ulong[_cubeWords];
            _forced = new ulong[_cubeWords];
            _used = new ulong[_cubeWords];

            _bestLength = GreedyLength();
        }

        /// <summary>Minimum covers as sorted indices of the cubes, without duplicates.</summary>
        public List<int[]> Solve()
        {
            var found = new HashSet<int[]>(IndexSetComparer.Instance);
            int maxSolutions = _maxSolutions;
            Run(maxSolutions: 1, columnDominance: true, nodes: -1);
            found.UnionWith(_solutions);
            Run(maxSolutions, columnDominance: false, nodes: TieSearchNodes);
            found.UnionWith(_solutions);
            return found.ToList();
        }

        private void Run(int maxSolutions, bool columnDominance, long nodes)
        {
            _maxSolutions = maxSolutions;
            _columnDominance = columnDominance;
            _nodesLeft = nodes;
            _solutions.Clear();
            var root = GetFrame(0);
            Bits.SetFirst(root.Pending, _cellCount);
            Bits.SetFirst(root.Allowed, _cubeCount);
            Branch(new List<int>(), 0);
        }

        private Span<ulong> Cover(int cube) => _cover.AsSpan(cube * _cellWords, _cellWords);
        private Span<ulong> Covering(int cell) => _covering.AsSpan(cell * _cubeWords, _cubeWords);
        private Span<ulong> Row(int row) => _rows.AsSpan(row * _cubeWords, _cubeWords);
        private Span<ulong> Column(int column) => _columns.AsSpan(column * _cellWords, _cellWords);

        private Frame GetFrame(int depth)
        {
            while (_frames.Count <= depth)
                _frames.Add(new Frame(_cellWords, _cubeWords, _cubeCount));
            return _frames[depth];
        }

        /// <summary>Size of a greedy cover: initial upper bound for the search.</summary>
        private int GreedyLength()
        {
            Span<ulong> pending = new ulong[_cellWords];
            Bits.SetFirst(pending, _cellCount);
            int size = 0;
            while (!Bits.IsEmpty(pending))
            {
                int best = 0, bestGain = -1;
                for (int i = 0; i < _cubeCount; i++)
                {
                    int gain = Bits.CountAnd(Cover(i), pending);
                    if (gain > bestGain || (gain == bestGain && _dashes[i] > _dashes[best]))
                    {
                        best = i;
                        bestGain = gain;
                    }
                }
                Bits.AndNot(pending, Cover(best));
                size++;
            }
            return size;
        }

        private bool IsFull => _maxSolutions > 0 && _solutions.Count >= _maxSolutions;

        /// <summary>
        /// Explores the node whose pending cells and allowed cubes are in the frame of
        /// <paramref name="depth"/>. Cubes forced by the reduction are appended to
        /// <paramref name="chosen"/> and removed again before returning.
        /// </summary>
        private void Branch(List<int> chosen, int depth)
        {
            if (_nodesLeft == 0)
                return;
            _nodesLeft--;
            var frame = _frames[depth];
            int chosenBefore = chosen.Count;
            int rowCount = Reduce(chosen, frame.Pending, frame.Allowed);
            if (rowCount >= 0)
            {
                if (Bits.IsEmpty(frame.Pending))
                {
                    Record(chosen);
                }
                else
                {
                    int needed = chosen.Count + LowerBound(rowCount);
                    if (needed < _bestLength || (needed == _bestLength && !IsFull))
                        BranchOnMostConstrainedCell(chosen, depth, rowCount);
                }
            }
            chosen.RemoveRange(chosenBefore, chosen.Count - chosenBefore);
        }

        private void BranchOnMostConstrainedCell(List<int> chosen, int depth, int rowCount)
        {
            var frame = _frames[depth];
            int bestRow = 0;
            for (int r = 1; r < rowCount; r++)
            {
                if (_rowCount[r] < _rowCount[bestRow])
                    bestRow = r;
            }
            // The scratch rows are overwritten by the children
            Row(bestRow).CopyTo(frame.BranchRow);

            int count = 0;
            for (int i = Bits.Next(frame.BranchRow, 0); i >= 0; i = Bits.Next(frame.BranchRow, i + 1))
            {
                int gain = Bits.CountAnd(Cover(i), frame.Pending);
                // Most new cells first, then bigger cubes, then index
                frame.BranchKeys[count] = ((long)(_cellCount - gain) << 40)
                                          | ((long)(32 - _dashes[i]) << 32)
                                          | (uint)i;
                frame.BranchCubes[count] = i;
                count++;
            }
            Array.Sort(frame.BranchKeys, frame.BranchCubes, 0, count);

            var child = GetFrame(depth + 1);
            for (int k = 0; k < count; k++)
            {
                int i = frame.BranchCubes[k];
                chosen.Add(i);
                frame.Pending.CopyTo(child.Pending, 0);
                Bits.AndNot(child.Pending, Cover(i));
                frame.Allowed.CopyTo(child.Allowed, 0);
                Branch(chosen, depth + 1);
                chosen.RemoveAt(chosen.Count - 1);
                // Covers using cube i were already explored
                Bits.Reset(frame.Allowed, i);
            }
        }

        /// <summary>
        /// Applies until nothing changes:
        ///   - forced cubes: the only candidate left for some cell
        ///   - row dominance: drop cell b if every cube covering some cell a also covers b
        ///     (covering a covers b)
        ///   - column dominance: drop cube p if a cube q covers a superset of its pending
        ///     cells with no more literals; equal coverage with equal literals is a tie and
        ///     both are kept
        /// Leaves the rows of the pending cells in the scratch buffers and returns how many
        /// there are, or -1 when some cell has no candidate left.
        /// </summary>
        private int Reduce(List<int> chosen, Span<ulong> pending, Span<ulong> allowed)
        {
            while (true)
            {
                int rowCount = 0;
                for (int j = Bits.Next(pending, 0); j >= 0; j = Bits.Next(pending, j + 1))
                {
                    var row = Row(rowCount);
                    Bits.And(row, Covering(j), allowed);
                    int count = Bits.Count(row);
                    if (count == 0)
                        return -1;
                    _rowCell[rowCount] = j;
                    _rowCount[rowCount] = count;
                    rowCount++;
                }

                Span<ulong> forced = _forced;
                forced.Clear();
                bool anyForced = false;
                for (int r = 0; r < rowCount; r++)
                {
                    if (_rowCount[r] == 1)
                    {
                        Bits.Or(forced, Row(r));
                        anyForced = true;
                    }
                }
                if (anyForced)
                {
                    for (int i = Bits.Next(forced, 0); i >= 0; i = Bits.Next(forced, i + 1))
                    {
                        chosen.Add(i);
                        Bits.AndNot(pending, Cover(i));
                    }
                    Bits.AndNot(allowed, forced);
                    continue;
                }

                Span<ulong> droppedCells = _droppedCells;
                droppedCells.Clear();
                bool anyCellDropped = false;
                for (int a = 0; a < rowCount; a++)
                {
                    for (int b = 0; b < rowCount; b++)
                    {
                        // A subset with the same size is an equal row: keep the first one
                        if (a == b || _rowCount[a] > _rowCount[b]
                                   || (_rowCount[a] == _rowCount[b] && a > b))
                            continue;
                        if (Bits.IsSubset(Row(a), Row(b)))
                        {
                            Bits.Set(droppedCells, _rowCell[b]);
                            anyCellDropped = true;
                        }
                    }
                }
                if (anyCellDropped)
                {
                    Bits.AndNot(pending, droppedCells);
                    continue;
                }

                int columnCount = 0;
                for (int i = Bits.Next(allowed, 0); i >= 0; i = Bits.Next(allowed, i + 1))
                {
                    var column = Column(columnCount);
                    Bits.And(column, Cover(i), pending);
                    _columnCube[columnCount] = i;
                    _columnCount[columnCount] = Bits.Count(column);
                    columnCount++;
                }
                Span<ulong> droppedCubes = _droppedCubes;
                droppedCubes.Clear();
                bool anyCubeDropped = false;
                for (int p = 0; p < columnCount; p++)
                {
                    if (_columnCount[p] == 0 || (_columnDominance && IsDominated(p, columnCount)))
                    {
                        Bits.Set(droppedCubes, _columnCube[p]);
                        anyCubeDropped = true;
                    }
                }
                if (anyCubeDropped)
                {
                    Bits.AndNot(allowed, droppedCubes);
                    continue;
                }

                return rowCount;
            }
        }

        private bool IsDominated(int p, int columnCount)
        {
            int dashesP = _dashes[_columnCube[p]];
            for (int q = 0; q < columnCount; q++)
            {
                if (q == p || _columnCount[q] < _columnCount[p])
                    continue;
                int dashesQ = _dashes[_columnCube[q]];
                // Being a superset, a different count means a different coverage
                bool better = dashesQ > dashesP
                              || (dashesQ == dashesP && _columnCount[q] != _columnCount[p]);
                if (better && Bits.IsSubset(Column(p), Column(q)))
                    return true;
            }
            return false;
        }

        /// <summary>
        /// Greedy independent set: cells that share no candidate cube each need a
        /// different cube, so their count is a valid lower bound.
        /// </summary>
        private int LowerBound(int rowCount)
        {
            for (int r = 0; r < rowCount; r++)
            {
                _rowOrderKeys[r] = ((long)_rowCount[r] << 32) | (uint)r;
                _rowOrder[r] = r;
            }
            Array.Sort(_rowOrderKeys, _rowOrder, 0, rowCount);

            Span<ulong> used = _used;
            used.Clear();
            int bound = 0;
            for (int k = 0; k < rowCount; k++)
            {
                var row = Row(_rowOrder[k]);
                if (!Bits.Intersects(row, used))
                {
                    Bits.Or(used, row);
                    bound++;
                }
            }
            return bound;
        }

        private void Record(List<int> chosen)
        {
            if (chosen.Count < _bestLength)
            {
                _bestLength = chosen.Count;
                _solutions.Clear();
            }
            if (chosen.Count == _bestLength && !IsFull)
            {
                var solution = chosen.ToArray();
                Array.Sort(solution);
                _solutions.Add(solution);
            }
        }

        private sealed class IndexSetComparer : IEqualityComparer<int[]>
        {
            public static readonly IndexSetComparer Instance = new();

            public bool Equals(int[]? x, int[]? y) => x.AsSpan().SequenceEqual(y);

            public int GetHashCode(int[] indices)
            {
                var hash = new HashCode();
                foreach (int i in indices)
                    hash.Add(i);
                return hash.ToHashCode();
            }
        }
    }

    /// <summary>Operations on bitsets stored as spans of <c>ulong</c> words.</summary>
    file static class Bits
    {
        public static int Words(int bits) => Math.Max(1, (bits + 63) >> 6);

        public static void Set(Span<ulong> bits, int i) => bits[i >> 6] |= 1UL << i;

        public static void Reset(Span<ulong> bits, int i) => bits[i >> 6] &= ~(1UL << i);

        /// <summary>Sets bits 0 to <paramref name="count"/> - 1 and clears the rest.</summary>
        public static void SetFirst(Span<ulong> bits, int count)
        {
            bits.Clear();
            int full = count >> 6;
            bits[..full].Fill(ulong.MaxValue);
            if ((count & 63) != 0)
                bits[full] = (1UL << count) - 1;
        }

        public static bool IsEmpty(ReadOnlySpan<ulong> bits) => bits.IndexOfAnyExcept(0UL) < 0;

        public static int Count(ReadOnlySpan<ulong> bits)
        {
            int count = 0;
            foreach (ulong word in bits)
                count += BitOperations.PopCount(word);
            return count;
        }

        public static int CountAnd(ReadOnlySpan<ulong> a, ReadOnlySpan<ulong> b)
        {
            int count = 0;
            for (int k = 0; k < a.Length; k++)
                count += BitOperations.PopCount(a[k] & b[k]);
            return count;
        }

        public static bool IsSubset(ReadOnlySpan<ulong> a, ReadOnlySpan<ulong> b)
        {
            for (int k = 0; k < a.Length; k++)
            {
                if ((a[k] & ~b[k]) != 0)
                    return false;
            }
            return true;
        }

        public static bool Intersects(ReadOnlySpan<ulong> a, ReadOnlySpan<ulong> b)
        {
            for (int k = 0; k < a.Length; k++)
            {
                if ((a[k] & b[k]) != 0)
                    return true;
            }
            return false;
        }

        public static void And(Span<ulong> result, ReadOnlySpan<ulong> a, ReadOnlySpan<ulong> b)
        {
            for (int k = 0; k < result.Length; k++)
                result[k] = a[k] & b[k];
        }

        public static void AndNot(Span<ulong> bits, ReadOnlySpan<ulong> other)
        {
            for (int k = 0; k < bits.Length; k++)
                bits[k] &= ~other[k];
        }

        public static void Or(Span<ulong> bits, ReadOnlySpan<ulong> other)
        {
            for (int k = 0; k < bits.Length; k++)
                bits[k] |= other[k];
        }

        /// <summary>Index of the first set bit at or after <paramref name="from"/>, or -1.</summary>
        public static int Next(ReadOnlySpan<ulong> bits, int from)
        {
            int k = from >> 6;
            if (k >= bits.Length)
                return -1;
            ulong word = bits[k] & (ulong.MaxValue << from);
            while (word == 0)
            {
                if (++k == bits.Length)
                    return -1;
                word = bits[k];
            }
            return (k << 6) + BitOperations.TrailingZeroCount(word);
        }
    }
}
