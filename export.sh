#!/bin/bash

if [ "$EUID" -eq 0 ]
then
  echo "Please run as user to export LB_LIBRARY_PATH in user space."
  exit 1
fi

INSTALL_DIR=/usr/local
LIB_DIR=lib

echo "Adding $INSTALL_DIR to LD_LIBRARY_PATH in .bashrc"
echo "export LD_LIBRARY_PATH=$INSTALL_DIR/$LIB_DIR:${LD_LIBRARY_PATH}" >> ~/.bashrc

source ~/.bashrc
