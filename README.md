# Introduction

Ce projet a été développé pour le cours de Réseaux S5 de l'UFR Math Info à Strasbourg. Le sujet est le suivant:

**Banque de données distribuée avec notions de respect de la qualité privée des données et tolérante aux pannes.**

Pour voir l'énoncé complet, cliquez [ici](enonce.pdf).

![Architecture du projet](schema.png)

# Auteurs

Danyl El-Kabir et François Grabenstaetter

# Licence

Ce projet a été développé sous Licence GNU-GPLv3

# Utilisation

## Compilation

Ce projet est composé de 4 modules: les outils dans common, le programme client, le serveur d'accès (relais), et le serveur de données (nœud). Il suffit de soit compiler chaque module séparément soit tout en un depuis la racine du projet:

```bash
make
```

## Mise en marche

Pour la mise en marche, il faut démarrer une instance du serveur d'accès (relais), puis des serveurs de données (nœuds) et des clients.

**Serveur d'accès:**
```bash
./main <port>
```

**Serveur de données:**
```bash
./main <relais_ip> <relais_port> <champ_stocké>
```

**Client:**
```bash
./main <relais_ip> <relais_port>
```

## Interface du client

Après s'être authentifié, on peut exécuter les requêtes suivantes:

- **lire** *<champ1>[,...]* (lire les informations auxquelles ont a accès uniquement)
- **ecrire** *<champ1>:<valeur1>[,...]* (modifier ses informations)
- **supprimer** (supprimer toutes ses informations)
- **bye** (quitter) ou CTRL+C
- **clear** (effacer l'écran actuel)
- **aide** (afficher l'aide)
