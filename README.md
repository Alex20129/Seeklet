# Description
**'Seeklet'** is an open-source search engine written in C++/Qt.
This project aims to create a truly decentralized, free and open-source search engine. Its core idea is to provide the general public with a P2P-based search mechanism that is inherently resistant to censorship.
Every participant runs a small node on ordinary hardware; together we build an independent, privacy-first alternative to centralized proprietary search services.

# Requirements
To build it you will need to install some development packages:

Qt WebEngine

`apt install qt6-webengine-dev`

Jansson

`apt install libjansson-dev`

HTMLcxx

`apt install libhtmlcxx-dev`

OpenDHT

`apt install libopendht-dev`

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
