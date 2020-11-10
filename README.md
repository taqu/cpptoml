# cpptoml
This is WIP.

Minimal implementation for [Tom's Obvious, Minimal Language v1.0.0-rc.3](https://toml.io/en/v1.0.0-rc.3).
"Minimal" means that I don't need kindful supports like, friendness for STL, error messages for incorrects, many allocations.

# Usage

# Limitations

- Support only less than 4G bytes of text.
- Support only less than 64K bytes elements (keys or values).

# Implementation notes

- In many cases, multiline-string, date-time or special-float are not used.

# Test

[BurntSushi/toml-test](https://github.com/BurntSushi/toml-test)

# License
This software is distributed under two licenses 'The MIT License' or 'Public Domain', choose whichever you like.

