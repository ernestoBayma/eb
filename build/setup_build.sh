#!/bin/bash

die() {
  echo "$*" >&2; exit 2;
}
needs_arg() { 
  if [ -z "$2" ]; then 
    die "No arg for --$1 option"; fi; 
}

get_macro_name_build() {
  macro="$1"
  ret=
  if [ "${macro,,}" == "release" ]; then 
    ret="OPTIMIZATION"
  else 
    ret="DEBUG"
  fi

  echo $ret
}

compare_versions() {
  command=$(echo -e "$1\n$2" | sort -V -r | head -n 1)
  if [[ "$command" = "$1" ]]; then
    if [[ "$1" != "$2" ]]; then
      echo "-1"
    else
      echo "0"
    fi
  else
    echo "1"
  fi

}

replace() {
  from="$1"
  to="$2"
  file="$3"
  
  if [ -z "$file" ]; then
      file=$filename
  fi
  sed -i 's|'"$from"'|'"$to"'|g' "$file"
}

usage() {
  >&2 cat << EOF
    [w | work_dir     ] - Path for the default folder where the webserver gonna save internals files.
    [r | root_dir     ] - Path for the default folder where requested files should be searched.
    [s | source_dir   ] - Path of folder of source code.
    [b | build_type   ] - String containing either release or debug.
    [p | program_name ] - Change default binary name.
EOF
exit 1
}

filename="Makefile.inc"
filename_bkp=".Makefile.inc.bkp"
new_filename="Makefile"
work_dir="$PWD"/../webserver
root_dir="$PWD"/../misc/www
program_name=coragem
source_dir="$PWD"/../code
build_type=release
version="0.0.1"
kernel_linux_ver=$(uname -r | grep -oP '\d+\.\d+\.\d+\.\d+')
epoll_compat_kernel=$( compare_versions "$kernel_linux_ver" "2.5.44" )
cpu_flags=$( cat /proc/cpuinfo | grep flags | uniq )

has_sse=$( echo -n "$cpu_flags" | grep sse )
has_sse=$?

has_avx=$( echo -n "$cpu_flags" | grep avx )
has_avx=$?

if [[ "$has_sse" -eq "0" ]]; then
    has_sse=1
else
    has_sse=0
fi

if [[ "$has_avx" -eq "0" ]]; then
    has_avx=1
else
    has_avx=0
fi

if [[ "$epoll_compat_kernel" -le "0" ]]; then
  epoll_compat_kernel=1
else
  epoll_compat_kernel=0
fi

args=$(getopt -a -o v:b:r:w:s:p:h --long work_dir:,root_dir:,help,source_dir:,program_name:,build_type: -- "$@")
if [[ "$?" -gt 0 ]]; then
    usage
fi

eval set -- ${args}
while :
do
  case "$1" in 
    -w | --work_dir)     work_dir="$2";      shift 2   ;;
    -r | --root_dir)     root_dir="$2";      shift 2   ;;
    -s | --source_dir)   source_dir="$2";    shift 2   ;;
    -p | --program_name) program_name="$2";  shift 2   ;;
    -b | --build_type)   build_type="$2";    shift 2   ;;
    -v | --version)      version="$2";       shift 2   ;;
    -h | --help)         usage ;             shift     ;;
    -- ) shift; break ;;
    *  ) die "Illegal option --$1";;
  esac
done

build_type=$(get_macro_name_build "$build_type")
options=( 
  "$work_dir"
  "$root_dir"
  "$program_name"
  "$source_dir"
  "$build_type"
  "$version"
  "$kernel_linux_ver"
  "$epoll_compat_kernel"
  "$has_sse"
  "$has_avx"
)
options_names=( 
  "Working directory"
  "Webserver root directory"
  "Executable name"
  "Source code directory"
  "Build type" 
  "Binary Version"
  "Kernel Version"
)
macros=(
  "{{DEFAULT_WORKING_DIR}}"
  "{{DEFAULT_ROOT_FOLDER}}"
  "{{SERVER_NAME}}"
  "{{SOURCE_CODE_FOLDER}}"
  "{{BUILD_TYPE}}"
  "{{VERSION}}"
  "{{LINUX_VER}}"
  "{{EPOLL_COMPAT_KERNEL}}"
  "{{HAS_SSE}}"
  "{{HAS_AVX}}"
)

printf "%-25s %s\n" "Option" "Value"
printf "%-25s %s\n" "------" "-----"

for i in "${!options_names[@]}"; do
  printf "%-25s %s\n" "${options_names[$i]}" "${options[$i]}"
done

echo "-------------------------------------"
echo "Configuration of Makefile is starting"
echo "Press Ctrl+C to Stop"
sleep 5

if [ ! -e "$filename_bkp" ]; then
  /bin/cp "$filename" "$filename_bkp"
fi

for i in "${!macros[@]}"; do
  used_option="${options[$i]}"
  macro="${macros[$i]}"
  if [ "$macro" = "{{BUILD_TYPE}}" ]; then
    if [ "$used_option" = "DEBUG" ]; then
        replace "{{DEBUG_TYPE}}" "-g" 
    else 
        replace "{{DEBUG_TYPE}}" ""
    fi
  fi

  replace "$macro" "$used_option" 
done

mv "$filename" "$new_filename"
mv "$filename_bkp" "$filename"

echo "Done."
