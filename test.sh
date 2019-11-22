#!/bin/bash
make

if [ $# -ne 1 ]; then
  echo "mauvais argument" > /dev/fd/2;
  exit 1;
fi

relais=$(echo $(pwd)"/relais")

noeud=$(echo $(pwd)"/noeud")

client=$(echo $(pwd)"/client")

gnome-terminal --tab \
  --working-directory=$relais --title='relais' --command="./main 2100"

gnome-terminal --tab \
  --working-directory=$noeud --title='age1' --command="./main 127.0.0.1 2100 age"

gnome-terminal --tab \
  --working-directory=$noeud --title='age2' --command="./main 127.0.0.1 2100 age"

gnome-terminal --tab \
  --working-directory=$noeud --title='taille1' --command="./main 127.0.0.1 2100 taille"

gnome-terminal --tab \
  --working-directory=$noeud --title='taille2' --command="./main 127.0.0.1 2100 taille"

for i in $(seq 1 $1); do
gnome-terminal --tab \
  --working-directory=$client --title="client$i" --command="./main 127.0.0.1 2100"
done
