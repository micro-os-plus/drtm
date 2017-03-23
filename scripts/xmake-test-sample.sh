# Included with `source` into `xpack-helper.sh`
# Is started in the folder where the test sources are.

# -----------------------------------------------------------------------------

# $1=target name
# $2=toolchain name
# $3=profile name
# ${xpack_name}=the linearised xPack name
# ${test_folder_path}=relative path to the test files
# ${xpack_root_path}=absolute path to xpack root

do_run_profile() {
    
  target_name=$(echo $1 | tr '[:upper:]' '[:lower:]')
  shift
  toolchain_name=$(echo $1 | tr '[:upper:]' '[:lower:]')
  shift
  profile_name=$(echo $1 | tr '[:upper:]' '[:lower:]')
  shift

  while [ $# -gt 0 ]
  do
    case "$1" in
      --)
        shift
        break
        ;;

      *)
        shift
        ;;
    esac
  done

  config_name="test-${test_name}-${target_name}-${toolchain_name}-${profile_name}"
  build_folder_path="build/${config_name}"
  build_folder_absolute_path="${xpack_root_path}/${build_folder_path}"
  # echo ${test_folder_path}
  
  # Project definitions; should be computed from JSONs.
  src_folders=( src samples ${test_folder_path} )
  include_folders=( include samples )
     
  common_opts="-Wall"
  artefact_name="${test_name}"
  if [ "${profile_name}" == "debug" ]
  then
    cxx_opts="-O0 -g3 -DDEBUG ${common_opts}"
    c_opts="${cxx_opts}"
    cpp_opts="-std=c++1y ${cxx_opts}"
  else
    cxx_opts="-O3 -g3 -DNDEBUG ${common_opts}"
    c_opts="${cxx_opts}"
    cpp_opts="-std=c++1y ${cxx_opts}"
  fi

  # The following should be set:
  # ${src_folders}
  # ${include_folders}
  # ${c_opts}
  # ${cpp_opts}

  # Prepare test variables & display configuration.
  do_xmake_test_begin

  # Generate make files for all source folders.
  do_xmake_generate
  
  # Initialy the current folder is where the test sources are.
  cd ${build_folder_absolute_path}

  if [ "${verbose}" == "y" ]
  then
    echo
    echo "Invoking builder: \"make $@\"..."
  fi
  make "$@"
  
  if [ -x "./${artefact_name}" ]
  then
    if [ "${verbose}" == "y" ]
    then
      echo
      echo "Invoking artefact: \"${artefact_name}\"..."
    fi
    echo
    ./${artefact_name}
  fi

  # Print a completion message.
  do_xmake_test_end
}

do_run() {
  
  echo "Starting test \"${test_name}\"..."
  echo
  
  xpack_name="drtm"
  # TODO: get profile names from JSON
  do_run_profile "${xmake_target_name}" "${xmake_toolchain_name}" debug -- "$@"
  do_run_profile "${xmake_target_name}" "${xmake_toolchain_name}" release -- "$@"
}

do_run "$@"
