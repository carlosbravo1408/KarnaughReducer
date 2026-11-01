using System.Numerics;

namespace KMapReducerWeb.Logic
{
    /// <summary>
    /// Product term in positional (PLA) notation. Bits set in <see cref="Mask"/> are the
    /// '-' positions; <see cref="Value"/> holds the fixed bits and is 0 where masked.
    /// In a SoP solution the cube groups ones; in a PoS solution it groups zeros,
    /// so it is always the group drawn on the map.
    /// </summary>
    /// <remarks>Ordered like its PLA string, with '-' &lt; '0' &lt; '1'.</remarks>
    public readonly record struct Cube(uint Value, uint Mask) : IComparable<Cube>
    {
        public int Dashes => BitOperations.PopCount(Mask);

        public static Cube FromPla(ReadOnlySpan<char> pla)
        {
            uint value = 0, mask = 0;
            foreach (char c in pla)
            {
                value <<= 1;
                mask <<= 1;
                if (c == '1')
                    value |= 1;
                else if (c == '-')
                    mask |= 1;
            }
            return new Cube(value, mask);
        }

        public string ToPla(int numInputs) =>
            string.Create(numInputs, this, static (chars, cube) =>
            {
                for (int i = 0; i < chars.Length; i++)
                {
                    uint bit = 1u << (chars.Length - 1 - i);
                    chars[i] = (cube.Mask & bit) != 0 ? '-' : (cube.Value & bit) != 0 ? '1' : '0';
                }
            });

        public bool Contains(int cell) => ((uint)cell & ~Mask) == Value;

        /// <summary>Cells of the group, without walking the whole map.</summary>
        public IEnumerable<int> Cells()
        {
            uint sub = Mask;
            while (true)
            {
                yield return (int)(Value | sub);
                if (sub == 0)
                    yield break;
                sub = (sub - 1) & Mask;
            }
        }

        public int Literals(int numInputs) => numInputs - Dashes;

        public int CompareTo(Cube other)
        {
            uint diff = (Value ^ other.Value) | (Mask ^ other.Mask);
            if (diff == 0)
                return 0;
            uint bit = 1u << (31 - BitOperations.LeadingZeroCount(diff));
            return Rank(bit).CompareTo(other.Rank(bit));
        }

        private int Rank(uint bit) => (Mask & bit) != 0 ? 0 : (Value & bit) != 0 ? 2 : 1;
    }
}
