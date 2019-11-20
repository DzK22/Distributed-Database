#!/bin/bash
make

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

gnome-terminal --tab \
  --working-directory=$client --title='client1' --command="./main 127.0.0.1 2100 francois lamachine"

gnome-terminal --tab \
  --working-directory=$client --title='client2' --command="./main 127.0.0.1 2100 bonsoir tata"
