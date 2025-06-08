def create_empty_grid(rows=128, cols=128):
    return [["0" for _ in range(cols)] for _ in range(rows)]


def place_pattern(grid, pattern_coords, top_left_row, top_left_col):
    for r, c in pattern_coords:
        rr = top_left_row + r
        cc = top_left_col + c
        if 0 <= rr < len(grid) and 0 <= cc < len(grid[0]):
            grid[rr][cc] = "1"


def write_grid_to_file(grid, filename):
    with open(filename, "w") as f:
        for row in grid:
            f.write("".join(row) + "\n")


# Define well known patterns as relative coordinates (row, col)
PATTERNS = {
    "glider": [(0, 1), (1, 2), (2, 0), (2, 1), (2, 2)],
    "small_exploder": [(1, 0), (0, 1), (1, 1), (2, 1), (0, 2), (2, 2), (1, 3)],
    "pulsar": [
        # Pulsar pattern (13x13 block)
        (2, 4),
        (2, 5),
        (2, 6),
        (2, 10),
        (2, 11),
        (2, 12),
        (4, 2),
        (4, 7),
        (4, 9),
        (4, 14),
        (5, 2),
        (5, 7),
        (5, 9),
        (5, 14),
        (6, 2),
        (6, 7),
        (6, 9),
        (6, 14),
        (8, 4),
        (8, 5),
        (8, 6),
        (8, 10),
        (8, 11),
        (8, 12),
        (10, 4),
        (10, 5),
        (10, 6),
        (10, 10),
        (10, 11),
        (10, 12),
    ],
    "blinker": [(0, 1), (1, 1), (2, 1)],
    "toad": [(1, 1), (1, 2), (1, 3), (2, 2), (2, 3), (2, 4)],
}


def generate_pattern(pattern_name: str, filename: str):
    rows, cols = 128, 128
    grid = create_empty_grid(rows, cols)
    pattern = PATTERNS.get(pattern_name.lower())

    if pattern is None:
        print(f"Pattern '{pattern_name}' not recognized!")
        return

    max_r = max(r for r, _ in pattern)
    max_c = max(c for _, c in pattern)
    start_row = (rows - max_r) // 2
    start_col = (cols - max_c) // 2

    place_pattern(grid, pattern, start_row, start_col)
    write_grid_to_file(grid, filename)
    print(f"Pattern '{pattern_name}' written to {filename}")


if __name__ == "__main__":
    generate_pattern("pulsar", "pulsar_128x128.txt")
    generate_pattern("glider", "glider_128x128.txt")
    generate_pattern("small_exploder", "small_exploder_128x128.txt")
    generate_pattern("blinker", "blinker_128x128.txt")
    generate_pattern("toad", "toad_128x128.txt")
