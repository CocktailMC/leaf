# NeoForge live (1.21.1)

```bash
# Ensure greeter.leafmod exists
./scripts/smoke-three-loaders.sh

cd bridge/neoforge-live
mkdir -p run/server/leafmods
cp -a ../../dist/leafmods/greeter.leafmod run/server/leafmods/
printf 'eula=true\n' > run/server/eula.txt

JAVA_HOME=~/.local/jdk25 ./gradlew runServer
```

Expect log lines:

```text
[LEAF][INFO] greeter on_load
[LEAF][INFO] greeter on_enable
...
Done (...s)! For help, type "help"
```
