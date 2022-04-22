

1. Hvordan superblokkfilen leses fra disk og lastes inn i minnet:
    Superblokken leses rekursivt fra disk ved hjelp av fil pekeren, hvor alle verdier blir satt i en nylig opprettet inode, og returnert fra metoden. Hvis inoden er et directory blir entries på angitt indeks satt til ett nytt kall på den rekursive metoden. Hvis inoden er en fil blir entries satt til oppføringene som ligger på superblokken.

2. Eventuelle implementasjonskrav som ikke er oppfylt:
    Ingen som jeg er klar over.

3. Eventuelle deler av implementasjonen som avviker fra prekoden: 
    Har gjort minimale endringer i prekoden, og bare lagt til ekstra hjelpemetode for rekursiv innlasting av superblokk, og sjekk for duplikater av samme navn i samme directory.

4. Eventuelle tester som feiler og hva som kan være årsaken: 
    Alle tester skal være ok, og uten feil i valgrind. 

5. Ekstra: 
    programmet kan kjøres fra prekode mappen ved hjelp av make runAll, som vil kjøre igjennom makefilene i alle testmappene, og kjøre testene ved med valgrind.
