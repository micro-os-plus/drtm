# The `sample` test

This test compiles a simple application that uses the files in the `samples` folder.

The project uses the include folders:

- `include`
- `samples`

and the source folders:

- `src`
- `samples`
- `tests/samples`

## Running the test

This test is automatically executed part of the xPack tests; both profiles (`debug` and `release`) are used.

To run the test individually, use

```bash
$ bash ../../scripts/xmake.sh test sample [--verbose]
```

The executable is also executed.

To clean a build:

```bash
$ bash ../../scripts/xmake.sh test sample [--verbose] -- clean
```


## Travis

This test is also executed by Travis in an Ubuntu container.
