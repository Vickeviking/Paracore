# playground/ — din verkstad

Varje `.cpp`-fil här blir ett **eget program** med en egen `main()`. Ingen
registrering, ingen redigering av Makefilen: `wildcard` plockar upp filen så
fort den finns.

```
make new PROG=minlek     # skapar playground/minlek.cpp från mall
make run PROG=minlek     # bygger och kör den
make list                # visar allt som ligger här
```

Utelämnar du `PROG` menas `hello`.

## Varför det här inte är ett test

Testerna i `tests/` måste vara deterministiska — de fäller grinden när de
faller, och ett test som ibland är rött lär dig att köra om i stället för att
läsa. Lekplatsen har inget sådant ansvar. Här får du skriva programmet som
hänger sig, mäta samma sak tre gånger och få tre svar, eller lämna en halv
tanke kvar över natten. Det är poängen med att de är skilda mappar.

Det betyder också att **inget här körs av `make check`**. En trasig fil i
playground/ fäller inte grinden — men den fäller `make` (allt byggs), vilket
är avsiktligt: kod som inte kompilerar ska synas direkt.

## Verktygen fungerar här också

Lekplatsen bygger mot samma bibliotek och samma flaggor som resten av repot,
så sanitizern ser din experimentkod precis som den ser bibliotekets:

```
make run PROG=x          # MODE=debug
make tsan-run PROG=x     # under ThreadSanitizer — kapplöpningar, låsordning
make asan-run PROG=x     # under ASan + UBSan — läckor, use-after-free, UB
make bench PROG=x        # release-bygge, -O2, för mätningar
```

`make tsan-run` är den du vill ha när ett experiment "fungerar ibland".
Det är nästan aldrig tur — det är nästan alltid en kapplöpning, och TSan
pekar på den i stället för att låta dig gissa.

## Det som redan ligger här

| fil | vad den visar |
|---|---|
| `hello.cpp` | att biblioteket lever, och vilka moduler som är byggda än så länge |
| `counter.cpp` | en oskyddad räknare mot en skyddad — kapplöpningen du kan se |
| `falsesharing.cpp` | falsk delning, `std::atomic_ref`, 19,4× på 8 trådar (mätt) |

De tre är skrivna för att läsas, inte bara köras. Börja med `counter.cpp` och
kör den under `make tsan-run PROG=counter`.

## Mappen är din

Ingenting i repot läser innehållet i playground/ utom Makefilen, och den bryr
sig bara om att filerna kompilerar. Radera, skriv om, strö skräpfiler — det
enda som är värt att veta är att filerna **committas** som allt annat. Vill du
ha något osparat, lägg det i `playground/scratch/`, som är git-ignorerad.
