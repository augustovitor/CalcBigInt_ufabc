#!/bin/sh
# compilar
make
# teste rápido (assumindo bin/calcbigint existente)
if [ -x bin/calcbigint ]; then
  echo "OK: build produced executable"
else
  echo "ERROR: build failed"
fi
