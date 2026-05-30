# Description
**'Seeklet'** is an open-source search engine written in C++/Qt.
This project aims to create a truly decentralized, free and open-source search engine. Its core idea is to provide the general public with a P2P-based search mechanism that is inherently resistant to censorship.
Every participant runs a small node on ordinary hardware; together we build an independent, privacy-first alternative to centralized proprietary search services.

# Requirements
To build it you will need to install some additional packages:

Qt WebEngine

```
apt install libqt6webenginecore6
apt install libqt6webenginewidgets6
apt install qt6-webengine-dev
apt install qt6-webview-dev
apt install qml6-module-qtwebengine
apt install qml6-module-qtwebengine-controlsdelegates
apt install qml6-module-qtwebview
```

HTMLcxx

```
apt install libhtmlcxx-dev
```

OpenDHT

```
apt install libopendht-dev
```

# Project roadmap:
+ Crawling
+ Indexing
+ Local index database: saving, loading
+ Basic search functionality: keyword search, TF-IDF
+ Multithreaded crawling
+ GUI
+ Configuration management
+ Basic P2P functionality: internode communication, search request propagation
+ Distributed index database: replication, merging, cross-validation
+ Extended search functionality: ranking, filtering, refining, image search
+ Extended P2P functionality: node bootstrapping, peer announce and discovery (OpenDHT / DHTNet), web of trust, node ratings, anonymization mechanism
