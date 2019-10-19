# Introduction

Ce projet a été développé pour le cours de Réseaux S5 de l'UFR Math Info à Strasbourg. Le sujet est: 

**Banque de données distribuée avec notions de respect de la qualité privée des données et tolérante aux pannes.**

Pour voir l'énoncé complet, cliquez [ici](enonce.pdf)

![Architecture du projet](schema.png)

# Auteurs

Danyl El-Kabir et François Grabenstaetter

# Licence

Ce projet a été développé sous Licence GNU-GPL3.0

# Utilisation

Ce projet est composé de 3 modules: le programme client, le relais, et le noeud. Il suffit de compiler chaque module séparemment en exécutant:

```bash
make
```

Pour la mise en marche, il faut démarrer une instance du serveur relais, puis des noeuds, puis enfin des clients.

**Serveur relais:** 
```bash
./main <port>
```

**Noeud de données:**
```bash
./main <relais_ip> <relais_port> <champ stocké> <fichier_données>
```

**Client:**
```bash
./main <relais_ip> <relais_port> <login> <mdp>
```

