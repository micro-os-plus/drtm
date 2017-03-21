# Included with `source` into `xpack-selper.sh`
# Runs in the test folder.

# pwd
# echo $@


# $1=target name
# $2=toolchain name
# $3=profile name
# ${xpack_name}=the linearised xPack name
# ${test_folder_path}=relative path to the test files
# ${xpack_root_path}=absolute path to xpack root
do_run_profile() {
    
  target_name=$(echo $1 | tr '[:upper:]' '[:lower:]')
  toolchain_name=$(echo $2 | tr '[:upper:]' '[:lower:]')
  profile_name=$(echo $3 | tr '[:upper:]' '[:lower:]')
  config_name="test-${test_name}-${target_name}-${toolchain_name}-${profile_name}"
  build_folder_path="build/${config_name}"
  build_folder_absolute_path="${xpack_root_path}/${build_folder_path}"
  # echo ${test_folder_path}
  
  # Project definitions; should be computed from JSONs.
  src_folders=( src ${test_folder_path} )
  include_folders=( include samples )
     
  common_opts="-Wall"
  artifact_name="${xpack_name}"
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

  do_xmake_test_begin

  # Root make files
  do_xmake_create_makefile
  do_xmake_create_objects
  do_xmake_create_sources
  
  # Root src files
  c_files=( "version" )
  cpp_files=( )
  do_xmake_create_subdir "src"
  
  # Test files
  c_files=( )
  cpp_files=( "main" )
  do_xmake_create_subdir "${test_folder_path}"
  
  # Initialy the current folder is where the test sources are.
  cd ${build_folder_absolute_path}
  echo
  echo "Invoking builder: \"make all\"..."
  make all
  
  echo
  echo "Invoking artefact: \"${artifact_name}\"..."
  echo
  ./${artifact_name}
  
  do_xmake_test_end
}

do_run() {
  echo "Starting test \"${test_name}\"..."
  echo
  
  xpack_name="drtm"
  # TODO: get profile names from JSON
  do_run_profile "${xmake_target_name}" "${xmake_toolchain_name}" debug
  do_run_profile "${xmake_target_name}" "${xmake_toolchain_name}" release
}

do_run
