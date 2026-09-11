# Recording to USB

In Settings → System, use the drive row’s Record button. Recordings are saved
under `Recordings` on the selected drive. Stop recording and wait for the saved
notification before removing the drive. A forced stop, write failure or lost
recording buffer shows an error; the partial file may be incomplete. A warning
appears when available space falls below 1 GiB. New takes preserve earlier audio
and cue files even when their timestamped names collide.

![Recording saved after WAV finalization](images/ui/0.0.8/recording-saved.png)

Synthetic desktop capture at 1024×600, Night theme, verified binary
`0.0.8-codex-v0-0-8-recording-fixes.3`. The notification appears after the encoder
and WAV file close successfully. The standard generated 128 BPM/124 BPM fixtures
are loaded; the capture does not show the Pi or personal tracks.

For regression coverage and hardware checks, see
[recording reliability](TESTING.md#recording-reliability).

Recording uses buffered file writes with kernel-managed writeback. Do not force
`sync_file_range` or cache eviction between encoder writes: even the WRITE-only
call can block on a full USB request queue and exhaust the recording FIFO.
The previous 1 MiB writeback cadence reproduced failures after about six seconds
of saved stereo WAV audio on the Pi. Finalization and write-error checks remain
in place; an arbitrarily slow or disconnected drive can still interrupt a take.

USB over-current/disconnect errors affect both storage and DDJ audio independently
of the recorder. Preserve the application and kernel logs before restarting;
verify stable hardware, then retest large USB WAV loads with the actual audio route.

Cue and rating lookups open per-drive SQLite stores read-only. On FAT media,
closing a write-enabled database can wait for pending drive writes even when
the lookup changed nothing, delaying loading and screen updates.


Deck replacement queues cue override saves on a background writer, including
Rekordbox loads. Pending values remain visible to immediate reloads; Safe Eject
waits for these writes and refuses to unmount if they fail. Selecting an uncached
row does not import the track until a load is requested. Metadata import and
Rekordbox ANLZ parsing can still delay a cold load; rating and sampler-bank saves
also retain synchronous paths.

The Pi image installs `99-bitedj-usb-storage.rules`, selecting BFQ and 64 software
requests for USB disks that support BFQ. A two-request queue reproduced playback
read starvation during recording, even after cue writes moved off the GUI thread.
Verify `/sys/block/<disk>/queue/scheduler` and `nr_requests` after reconnecting or
rebooting. This improves playback read service; it cannot make faulty media or
USB power reliable. See the [kernel BFQ documentation](https://cdn.kernel.org/doc/html/latest/block/bfq-iosched.html).

Always stop recording and wait for the saved notification before Safe Eject or
shutdown. Forced termination can leave a WAV header incomplete even when audio
was written; an internal master recording also cannot establish the quality of
the DDJ's analog output.
