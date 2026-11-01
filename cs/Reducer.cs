namespace KMapReducerWeb.Logic.QuineMcCluskeyAlgorithm
{
    /// <summary>
    /// Exact two-level minimizer: Quine-McCluskey prime implicants, essential primes and
    /// an exact minimum cover of the remaining cells (<see cref="CoverSearch"/>).
    /// </summary>
    public class Reducer
    {
        /// <summary>
        /// Up to <paramref name="maxSolutions"/> minimal solutions (fewest cubes, sorted by
        /// literals), or all of them when <paramref name="maxSolutions"/> &lt;= 0. Every cube
        /// groups ones when <paramref name="isSop"/> is true and zeros otherwise; the flag
        /// applies to the whole result, not to each cube.
        /// </summary>
        public IReadOnlyList<IReadOnlyList<Cube>> GetAllSolutions(
            int numInputs,
            IEnumerable<int> ones,
            IEnumerable<int> notCare,
            bool isSop = true,
            int maxSolutions = 5)
        {
            var notCareSet = new HashSet<int>(notCare);
            // Cells to cover: ones in SoP, zeros in PoS
            var targets = new HashSet<int>(ones);
            if (!isSop)
            {
                var zeros = Enumerable.Range(0, 1 << numInputs).ToHashSet();
                zeros.ExceptWith(targets);
                zeros.ExceptWith(notCareSet);
                targets = zeros;
            }
            var values = new HashSet<int>(targets);
            values.UnionWith(notCareSet);

            var primes = GetPrimeImplicants(numInputs, values);
            var essentials = GetEssentialPrimeImplicants(primes, targets);
            var pending = targets
                .Where(cell => !essentials.Any(cube => cube.Contains(cell)))
                .Order()
                .ToList();
            // Bigger cubes first: good solutions show up early and tighten the bound
            var candidates = primes
                .Where(cube => !essentials.Contains(cube) && pending.Any(cube.Contains))
                .OrderByDescending(cube => cube.Dashes)
                .ThenBy(cube => cube.Value)
                .ThenBy(cube => cube.Mask)
                .ToList();

            var solutions = new CoverSearch(candidates, pending, maxSolutions)
                .Solve()
                .Select(cover => essentials
                    .Concat(cover.Select(i => candidates[i]))
                    .Order()
                    .ToArray())
                .ToList();
            // Phase 1 of the search may add one solution beyond maxSolutions: keep the
            // ones with fewer literals
            solutions.Sort((a, b) =>
            {
                int byLiterals = a.Sum(c => c.Literals(numInputs))
                    .CompareTo(b.Sum(c => c.Literals(numInputs)));
                return byLiterals != 0 ? byLiterals : a.AsSpan().SequenceCompareTo(b);
            });
            if (maxSolutions > 0 && solutions.Count > maxSolutions)
                solutions.RemoveRange(maxSolutions, solutions.Count - maxSolutions);
            return solutions;
        }

        /// <summary>
        /// Quine-McCluskey with hash-based neighbor lookup: O(n * m) per level. Two cubes
        /// merge only if they share the mask and differ in exactly one fixed bit.
        /// </summary>
        private static List<Cube> GetPrimeImplicants(int numInputs, HashSet<int> values)
        {
            uint allInputs = (1u << numInputs) - 1;
            var cubes = values.Select(v => new Cube((uint)v, 0)).ToHashSet();
            var merged = new HashSet<Cube>();
            var combined = new HashSet<Cube>();
            var primes = new List<Cube>();
            while (cubes.Count > 0)
            {
                merged.Clear();
                combined.Clear();
                foreach (var cube in cubes)
                {
                    for (uint free = allInputs & ~(cube.Value | cube.Mask); free != 0; free &= free - 1)
                    {
                        uint bit = free & (~free + 1);
                        var neighbor = new Cube(cube.Value | bit, cube.Mask);
                        if (cubes.Contains(neighbor))
                        {
                            merged.Add(new Cube(cube.Value, cube.Mask | bit));
                            combined.Add(cube);
                            combined.Add(neighbor);
                        }
                    }
                }
                foreach (var cube in cubes)
                {
                    if (!combined.Contains(cube))
                        primes.Add(cube);
                }
                (cubes, merged) = (merged, cubes);
            }
            return primes;
        }

        private static HashSet<Cube> GetEssentialPrimeImplicants(List<Cube> primes, HashSet<int> targets)
        {
            var essentials = new HashSet<Cube>();
            foreach (int cell in targets)
            {
                Cube only = default;
                int count = 0;
                foreach (var cube in primes)
                {
                    if (cube.Contains(cell))
                    {
                        only = cube;
                        if (++count > 1)
                            break;
                    }
                }
                if (count == 1)
                    essentials.Add(only);
            }
            return essentials;
        }
    }
}
