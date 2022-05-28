# cpptoml
This is WIP.

A minimal reader of [Tom's Obvious, Minimal Language v1.0.0](https://toml.io/en/v1.0.0).
"Minimal" means that this doesn't have kindful supports like, friendness for STL, error messages for incorrects,
but aim for minimal memory footpoints, supports of yourown allocators.

# Usage

# Limitations
- Support only less than 4G bytes of text.
- Uncheck the number of nests, so deep nests of tables or arrays will cause stackoverflow.

# Test
Use test cases [BurntSushi/toml-test](https://github.com/BurntSushi/toml-test).

# ToDo

- Pass the test of bad Unicode's codepoints
- Write comments and document

# License
This software is distributed under two licenses 'The MIT License' or 'Public Domain', choose whichever you like.

