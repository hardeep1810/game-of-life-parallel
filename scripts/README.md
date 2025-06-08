
# Pattern Generation Script

This repository contains a Python script for generating and placing well-known cellular automaton patterns onto a grid. The patterns can be written to text files for visualization. For more 
information see [Wikipedia: Conways Game Of Life: Examples of patterns](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life).

## Introduction

The script is designed to generate predefined patterns for cellular automata, such as a Glider, Small Exploder, Pulsar, Blinker, and Toad. 

## Usage

Run the script from the command line to generate patterns. For example:

```bash
python generate_patterns.py
```

This command will generate the pattern files for all predefined patterns and save them in the current directory.

## Available Patterns

The script can generate the following patterns:

- **Glider**: A small, self-propagating pattern.
- **Small Exploder**: A pattern that quickly spans its surroundings.
- **Pulsar**: A larger, periodic pattern that cycles every 3 generations.
- **Blinker**: A basic oscillator with a period of 2.
- **Toad**: Another simple oscillator with a period of 2.

