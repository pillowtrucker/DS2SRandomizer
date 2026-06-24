This are the main files that write the param files for the randomizer.

This is not the full source code, I ommited the gui and some other minor parts.

THIS IS NOT GOOD CODE
When I started the project I mostly wanted to code things fast to be able to test them.
As time went on I kept building and patching on top so is a mess now. Specially the enemy tables are kinda weird.

## Building

```
make            # builds ds2rando_cli (headless runner)
make lib        # builds libds2rando.a (core only)
make clean
```

Requirements: a C++17 compiler (tested with g++ 13). No third-party libraries —
only the C++ standard library.

Notes on this source drop:
- `modules/utils.hpp` was not included in the original drop and has been
  reconstructed from how the rest of the code uses it (file/parse/random/vector
  helpers, `Stopwatch`, `hash_str_uint32`). The random-helper namespace is named
  `rng` rather than `random` because POSIX `random()` collides with it on
  Linux/libstdc++.
- `param_editor.cpp` is an older duplicate of the templates in
  `modules/param_editor.hpp` and is not compiled.
- The enemy-table text loaders were updated to match the `;`-delimited data
  format shipped with the binary release.

The GUI is not part of this drop. `cli_main.cpp` is a small headless driver:
run it from a directory containing the release's `data/` folder; it reads
`er_config.txt` and writes the result to `Param/`.

```
./ds2rando_cli [seed]
```

## Shuffle (enemies & bosses)

By default the randomizer *randomizes*: every spawn independently picks a random
enemy (and arenas get random bosses), which distorts the population — many
enemies disappear while others get duplicated all over. The *shuffle* modes
instead permute what is already placed, so the multiset is preserved: every
enemy/boss still appears exactly as many times as in the vanilla game, just in
different spots.

Enable it in `er_config.txt`:

```
#SHUFFLE_ENEMIES 1   # 1 = shuffle existing enemies, 0 = original randomize behavior
#SHUFFLE_GLOBAL 1    # 1 = shuffle enemies across the whole game, 0 = within each map only
#SHUFFLE_BOSSES 1    # 1 = shuffle the arena bosses, 0 = original randomize behavior
```

Boss shuffle permutes the arena bosses while respecting the vanilla size
constraint (a boss only goes where it fits). A handful of arenas can't take part
in a population-preserving permutation — special multi-entity/gimmick encounters
that aren't standard single bosses (the Congregation, the Shulva gank trio, the
bow half of the Twin Dragonrider) and bosses excluded by `#BANNED` (e.g. the
Ancient Dragon, which is too big for any arena anyway). Those arenas fall back to
the original randomizer behaviour: they receive a random boss rather than keeping
their vanilla one. Set `DS2_BOSS_DEBUG=1` in the environment to log which arenas
were randomized and why.

Each run prints a short population report so you can confirm the shuffle kept the
enemy and boss counts identical to the original.
