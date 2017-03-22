#!/usr/bin/env bash

# -----------------------------------------------------------------------------
# Safety settings (see https://gist.github.com/ilg-ul/383869cbb01f61a51c4d).

if [[ ! -z ${DEBUG} ]]
then
  set ${DEBUG} # Activate the expand mode if DEBUG is anything but empty.
else
  DEBUG=""
fi

set -o errexit # Exit if command failed.
set -o pipefail # Exit if pipe failed.
set -o nounset # Exit if variable not set.

# Remove the initial space and instead use '\n'.
IFS=$'\n\t'

# -----------------------------------------------------------------------------

script=$0
if [[ "${script}" != /* ]]
then
  # Make relative path absolute.
  script=$(pwd)/$0
fi

parent="$(dirname ${script})"
# echo $parent

# -----------------------------------------------------------------------------
# Until the xmake-js tool will be functional, use this Bash script
# to build and test xPacks.
# -----------------------------------------------------------------------------

function do_run()
{
  echo "\$ $@"
  "$@"
}

# -----------------------------------------------------------------------------

xpack_helper_path="${HOME}/Downloads/xpack-sh.git/xpack-helper.sh"

if [ ! -f "${xpack_helper_path}" ]
then
  mkdir -p "${HOME}/Downloads"
  do_run git clone --depth 3 https://github.com/xpack/xpack-sh.git "${HOME}/Downloads/xpack-sh.git"
fi

source  "${xpack_helper_path}"

# -----------------------------------------------------------------------------

# Forward the args to the helper.
do_xmake $@

# -----------------------------------------------------------------------------
