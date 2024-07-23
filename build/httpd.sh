#!/bin/bash

start() {
    echo "Starting..."
    $PWD/coragem --config "/home/ernesto/coragem/misc/server_config" "$@" &
    echo "Done."
}

kill() {
  if [ ! -f "coragem.pid" ]; then
    pid=$( ps aux | grep -v "grep" | grep -e ".*coragem --config.*" | sed 's/[[:space:]]+/ /g' | cut -d " " -f 5 )
    pid=$(echo "$pid" | awk '{$1=$1};1')
    
  else
      pid=`cat "coragem.pid"`
  fi

  if [ ! -z "$pid" ]; then
        echo "Trying to kill program of pid "$pid"."
        kill -9 "$pid"
        rm coragem.pid
        echo "Done."
    fi
}

case "$1" in
  start)
      shift
      start "$@"
      exit
    ;;
   stop)
      shift
      kill  
      exit
    ;;
  restart)
      shift
      kill 
      start "$@"
      exit
    ;;
   *)
      echo "httpd.sh [start|stop|restart]"
      exit
    ;;
esac
