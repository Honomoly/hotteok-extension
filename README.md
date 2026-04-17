# hotteok

An ordered unique container with O(1) index lookup, implemented as a CPython C extension.

## Installation

```bash
pip install hotteok
```

Requires Python 3.12 or later.

## Quick Start

```python
from hotteok import UniqueList

ul = UniqueList([1, 2, 3])

ul.append(4)           # O(1)
ul.append(1)           # no-op — already present

ul.index(4)            # O(1)  — unlike list.index (which is O(n))
3 in ul                # O(1)  — unlike list.__contains__

ul.remove(2)           # O(1) lookup + O(n) shift
print(list(ul))        # [1, 3, 4]
```

Constructing from any iterable is supported, with duplicates silently skipped while preserving first-seen order:

```python
UniqueList([1, 2, 2, 3, 1])   # UniqueList([1, 2, 3])
UniqueList(x * x for x in range(5))   # UniqueList([0, 1, 4, 9, 16])
```

## Why UniqueList?

`UniqueList` sits between `list` and `set`:

- **Ordered like `list`** — insertion order is preserved and indexable.
- **Unique like `set`** — duplicates are rejected (or ignored, depending on the operation).
- **O(1) `index(x)` and `x in ul`** — via an internal `item → index` mapping.
- **Written in C** — no Python-level wrapping overhead on the hot path.

Use it when you need all three at once: stable order, uniqueness, and fast membership / index lookup.

## API Overview

`UniqueList` implements the standard mutable sequence protocol:

| Category   | Methods / operators |
|------------|---------------------|
| Construction | `UniqueList()`, `UniqueList(iterable)` |
| Add        | `append(x)`, `extend(iter)`, `insert(i, x)` |
| Remove     | `pop([i])`, `remove(x)`, `del ul[i]` |
| Access     | `ul[i]`, `ul[i] = x`, `len(ul)`, `x in ul` |
| Query      | `index(x)` |
| Copy       | `copy()`, `tolist()` |
| Iteration  | `iter(ul)`, `reversed(ul)` |

### Duplicate handling

- `append(x)` with an existing `x`: **silently ignored** (no-op).
- `extend(iter)`: duplicates within the iterable are skipped.
- `insert(i, x)` or `ul[i] = x` with an existing `x`: **raises `ValueError`**.

### Unhashable values

Since uniqueness is enforced via hashing, unhashable values (e.g. `list`, `dict`) raise `TypeError` on insertion or lookup — same behavior as `set`.

## Development

This package is built from C sources and requires a C compiler and Python development headers.

```bash
git clone https://github.com/Honomoly/hotteok-extension.git
cd hotteok-extension
uv pip install -e .
uv run pytest tests/
```

The C sources live under `src/hotteok/_core/`. The Python-facing type stubs are in `src/hotteok/_core.pyi`.

## Project Status

`hotteok` is a **learning project** — built as a way to explore the CPython C API in depth (reference counting, type objects, sequence protocol, hash-table integration). It is usable and fully tested, but the API may evolve and more types may be added as the exploration continues.

Performance characteristics are not yet formally documented; once further optimization work lands, a benchmarks section will be added here.

## License

MIT — see [LICENSE](LICENSE).
