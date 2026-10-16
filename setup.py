from setuptools import setup, Extension

setup(
    ext_modules=[
        Extension(
            "hotteok._core",
            sources=[
                "src/hotteok/_core/_core.c",
                "src/hotteok/_core/uniquelist/uniquelist_type.c",
            ],
            include_dirs=[
                "src/hotteok/_core",
                "src/hotteok/_core/uniquelist",
            ],
            extra_compile_args=[
                "/utf-8"
            ]
        )
    ],
)
