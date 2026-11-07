# Kathará practice and live workstation

Run `kathara lstart --shared` from this directory after the course staff has confirmed the lab image, compiler, zlib development files, and `/shared` mount on the class machines. The lab defines one collision domain and two devices:

- `pc1`: `10.0.0.1/24` on `eth0`
- `pc2`: `10.0.0.2/24` on `eth0`

The `shared/` folder in this lab directory is mounted as `/shared` inside both devices. For a practice run after you implement FieldDrop, run these commands from the project root on the host:

```sh
sh kathara/prepare_shared.sh
cd kathara
kathara lstart --shared
```

Verify addresses with `ip address` and connectivity with `ping` inside the devices. In the `pc2` terminal, use a fresh output path:

```sh
cd /shared/fielddrop
make
./fielddrop-recv /tmp/fielddrop-output.bin 43000
```

In the `pc1` terminal:

```sh
cd /shared/fielddrop
./fielddrop-send input.bin 10.0.0.2 43000
sha256sum input.bin
```

After the receiver exits, run `sha256sum /tmp/fielddrop-output.bin` in `pc2` and compare the hashes. Run `kathara lclean` from the `kathara/` directory on the host when finished. Remove `/tmp/fielddrop-output.bin` before another run. The receiver's six-second wait after a successful `FIN` is expected.

For the assessed workstation visit, extract an identical copy of your submitted group ZIP and use a **clean copy** of the starter lab. From the starter root, run `sh kathara/stage_submission.sh PATH_TO_EXTRACTED_GROUP_FOLDER`; it stages only the submitted Makefile and `source/` and refuses an existing stage. From `kathara/`, run `kathara lstart --shared`. Build inside a device and confirm `pc1` and `pc2` are ready, but leave sender and receiver stopped. The marker will generate a fresh file under `pc1`'s local `/tmp`, select a fresh output path under `pc2`'s local `/tmp`, and ask the group to run the transfer. The marker may require a clean rebuild from the submitted ZIP. The earlier practice commands using `/shared/fielddrop/input.bin` are not the assessed input path. Do not run `prepare_shared.sh` for the assessed visit.

The lab pins the course image tag `fielddrop-course:1.0`. The instructor builds this image from private course materials before release. A successful `kathara lstart --print` checks configuration syntax only; the live lab must be tested on a course machine.
