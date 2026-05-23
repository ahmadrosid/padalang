# Padalang

Padalang is a programming language that uses **Minang Kabau** for its syntax. Programs should read naturally to Minang speakers — like describing logic in everyday language.

## Example

```padalang
# Halo urang!
caliak "Halo, Urang!"

simpan namo = "Ahmad"
simpan umua = 25

bilo umua >= 17 baru
    caliak namo + " — urang dewasa"
indak
    caliak namo + " — umua masiak kanak-kanak"

fungsi hitung(n)
    bilo n > 10 baru
        bulian "banyak"
    indak
        bulian "saketek"

caliak hitung(15)

untuak i dari 1 sampai 5
    caliak i
```

## Keywords

| Concept  | Padalang |
| -------- | -------- |
| print    | `caliak` |
| assign   | `simpan` |
| if       | `bilo`   |
| then     | `baru`   |
| else     | `indak`  |
| function | `fungsi` |
| return   | `bulian` |
| for      | `untuak` |
| and      | `jo`     |
| or       | `atau`   |
| not      | `indak`  |


## REPL

Start the interactive REPL:

```bash
./padalang
# or
./padalang --repl
```

```
padalang> caliak "Halo, Urang!"
Halo, Urang!
padalang> simpan x = 10
padalang> caliak x
10
```

Multi-line blocks use the `....>` continuation prompt:

```
padalang> bilo x > 5 baru
....>     caliak "banyak"
padalang> exit
```

Parse and print the AST:

```bash
./padalang --parse examples/hello.pad
```

Run the lexer smoke test:

```bash
bash tests/test_lexer.sh
```

Run the parser smoke test:

```bash
bash tests/test_parser.sh
```

Run the REPL smoke test:

```bash
bash tests/test_repl.sh
```

Run a program:

```bash
bash tests/test_run.sh
```
