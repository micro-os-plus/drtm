# File templates

## `version.h`

This template is used to generate the `include/drtm/version.h` file, based on the xPack version in `package.json`.

Until the `xpm` tool will be available to fully process the liquid tags and filters, this file is processed by the separate shell script `scripts/version.sh`, invoked by `npm`.

