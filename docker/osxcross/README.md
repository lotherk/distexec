# get it
```
docker pull lotherk/distexec-osxcross
```

# build it
```
export RELEASE_DIR=/tmp/distexec_release
mkdir -p $RELEASE_DIR
docker run --rm -i -v $RELEASE_DIR:/releases lotherk/distexec-osxcross
```
