WANStreamer Engine
==================

WANStreamer is a reliable, one-to-one streaming engine for sending ADIOS2
steps over a TCP connection.  The consumer creates the listening socket; the
producer connects to it.  WANStreamer is independent of DataMan and ZeroMQ.
It is built when ADIOS2 has Sodium support.

Both applications select the same engine name::

  io.SetEngine("WANStreamer");

Step acknowledgement
--------------------

WANStreamer deliberately gives ``EndStep()`` an additional commit meaning on
the reader:

* Writer ``EndStep()`` copies the complete step into a bounded in-memory replay
  queue. It normally returns immediately; it applies backpressure when that
  queue is full.
* Reader ``EndStep()`` records the step as committed and sends the
  acknowledgement.

Consequently, an application that copies WANStreamer input to BP5 must call
the BP5 writer's ``EndStep()`` before the WANStreamer reader's ``EndStep()``.
This contract provides acknowledgement without extending the ADIOS2 API.

If a connection fails before acknowledgement, the writer retains the step,
reconnects, authenticates again, and retransmits it.  Every connection
exchanges a random stream ID and a monotonically increasing step ID.  The
reader checkpoints its last committed ID and suppresses retransmitted
duplicates.

The sender thread removes a step only after the matching reader
acknowledgement. ``Close()`` drains all queued steps before closing the stream.
Thus producer ``EndStep()`` means "accepted by WANStreamer's replay queue",
whereas reader ``EndStep()`` means "consumer application committed and
acknowledged". A sender failure is reported by a subsequent writer
``EndStep()`` or ``Close()``.

This is an at-least-once transport with commit-aware duplicate suppression.
A process failure in the small interval after BP5 commits but before the
WANStreamer reader checkpoints its ``EndStep()`` can result in the application
writing that step again after restart.  No queued step is discarded while the
writer process remains alive, but WANStreamer cannot make BP5 and its own
checkpoint one atomic transaction. The replay queue is memory-resident: a
producer process or host failure loses steps that have not yet been
acknowledged. Applications requiring survival of producer failure need a
persistent upstream source or a future disk spool.

ZeroMQ
------

ZeroMQ could provide the underlying sockets, reconnect machinery, message
framing, and an efficient asynchronous in-memory queue. Those features would
reduce transport plumbing and offer useful high-water-mark controls. They do
not replace WANStreamer's replay protocol: a ZeroMQ send completion does not
mean that the consumer completed BP5 ``EndStep()``, and an in-memory ZeroMQ
queue does not survive producer failure. Stream IDs, step IDs, reader
acknowledgements, persistent consumer checkpoints, and replay would still be
required. The current implementation uses TCP directly to avoid requiring
ZeroMQ; adopting it later would be a transport substitution rather than the
ring-buffer reliability design.

Writer parameters
-----------------

``ConnectionFile`` (required)
  Encrypted rendezvous file produced by the reader.  For an MPI communicator,
  use ``{rank}`` in the path.  If it is omitted, WANStreamer appends ``.RANK``.

``PrivateKey`` (required)
  Raw 32-byte Curve25519 private key.  It decrypts the rendezvous and proves
  the writer's identity to the reader.

``LaunchMode``
  ``none`` (default) waits for an independently launched reader. ``system``
  runs ``LaunchCommand`` initially and after repeated reconnect failures.

``LaunchCommand``
  Trusted shell command used by ``LaunchMode=system``.  WANStreamer replaces
  ``{connection_file}``, ``{rank}``, and ``{stream_id}``.  The command may use
  OpenSSH, a batch scheduler, or a site launcher.  OpenSSL cannot launch SSH
  processes because it does not implement the SSH protocol.

``RetryIntervalSeconds``
  Delay between connection attempts. Default: ``1``.

``ReconnectTimeoutSeconds``
  Maximum time the sender thread retries an unacknowledged queue. ``0``
  (default) retries without a deadline. A positive timeout records a sender
  error only after the retry deadline, not on an individual disconnect. The
  application observes it at a writer ``EndStep()`` or ``Close()``.

``ReplayBufferSteps``
  Maximum number of complete, unacknowledged steps retained by the writer.
  Default: ``64``. Writer ``EndStep()`` waits instead of dropping a step when
  this limit is reached.

``ReplayBufferBytes``
  Optional byte limit for queued variable payloads. ``0`` (default) disables
  the byte limit. A single step is accepted into an empty queue even if it is
  larger than this value, avoiding a permanent wait.

Reader parameters
-----------------

``ConnectionFile`` (required)
  Path where the encrypted rendezvous is atomically published.

``PublicKey`` (required)
  Raw 32-byte Curve25519 public key used to encrypt the rendezvous and
  authenticate the connecting writer.

``BindAddress``
  Local address on which the reader listens. Default: ``0.0.0.0``.

``AdvertiseAddress``
  Address written to the rendezvous file. Default: ``127.0.0.1``. Set this to
  a hostname or address reachable from the producer.

``Port``
  Listening TCP port. ``0`` (default) requests an available ephemeral port.

``CheckpointFile``
  Persistent commit checkpoint. Default: ``ConnectionFile + ".checkpoint"``.
  This file is intentionally retained when the reader closes so a relaunched
  reader can suppress a replay of its last committed step.

Supported data and MPI model
----------------------------

WANStreamer currently supports numeric primitive and complex variables with
full-variable reads. String variables and reader-side sub-selections are not
supported. Each MPI writer rank has one corresponding reader rank and one TCP
connection. WANStreamer does not perform M-to-N redistribution.

Security
--------

The rendezvous contains the consumer ID, host, port, protocol version, and
rank. It is encrypted with a Sodium Curve25519 sealed box. The reader also
challenges every connection to prove possession of the matching private key.
The data stream itself is not encrypted. Use a protected network, VPN, or SSH
tunnel when payload confidentiality is required.
