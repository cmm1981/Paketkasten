# Software Download und Build
```sh
git clone https://github.com/cmm1981/Paketkasten.git app
west init -l app
west update
west zephyr-export
west build -b paketkasten app -- -DBOARD_ROOT=.
```

# Software flashen
`west flash`

