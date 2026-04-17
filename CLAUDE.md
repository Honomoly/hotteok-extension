# hotteok

CPython C extension package. Korean-speaking user (learning-focused — prefer detailed explanations for C extension topics).

## Build

```sh
uv pip install -e .
```

Rebuilds the C extension in-place. Run after any `.c` / `.h` change before testing.

## Test

```sh
uv run pytest tests/**
```

## Layout

- `src/hotteok/` — Python package
- `src/hotteok/_core/` — C sources (compiled into `_core.*.so`)
- `src/hotteok/_core.pyi` — type stubs for the C extension
- `tests/` — pytest suite
