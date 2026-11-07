# FieldDrop live workstation readiness

Bring an identical copy of the ZIP you submitted. The marker may compare it with the staged source and ask for a clean rebuild. You cannot replace the assessed code during the visit.

Before the marker reaches your workstation:

- [ ] Extract your submitted ZIP on the university Linux PC. Confirm `make clean` and `make` work from that extraction.
- [ ] Use a clean starter lab. From its root, run `sh kathara/stage_submission.sh PATH_TO_EXTRACTED_GROUP_FOLDER` to stage **that ZIP's** Makefile and `source/` in `/shared/fielddrop/`. Do not run the practice helper or stage a test input.
- [ ] From the lab directory run `kathara lstart --shared`; confirm `pc1` is `10.0.0.1/24`, `pc2` is `10.0.0.2/24`, and ping works. Build the staged code inside a device.
- [ ] Have a terminal ready in each device. Leave the assessed sender and receiver stopped until the marker arrives.
- [ ] Have your submitted test results, one explained fault trace, report, and contributions available to discuss.

During the visit, the marker creates a fresh input in `pc1`'s local `/tmp` and chooses a fresh output path in `pc2`'s local `/tmp`. Start the receiver on `pc2`, then the sender on `pc1`, and compare SHA-256 hashes in the two devices. Each member should be ready to explain their own work, the packet exchange, and the submitted fault evidence. A live fault injection is not required; the marker checks fault and reference-peer cases separately from the submitted ZIP.
