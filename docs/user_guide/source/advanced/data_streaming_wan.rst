WAN Data Streaming with WANStreamer
===================================

This pattern moves interferometer snapshots to a remote process that commits
them to BP5.  Python only coordinates ADIOS2 engines; sockets, authentication,
reconnect, replay, and acknowledgements stay inside WANStreamer.

Consumer
--------

Start the consumer first when ``LaunchMode=none``. It creates a WANStreamer
reader and a normal BP5 writer::

  wan_io = adios.declare_io("WANInput")
  wan_io.set_engine("WANStreamer")
  wan_io.set_parameters({
      "ConnectionFile": connection_file,
      "PublicKey": public_key,
      "BindAddress": "0.0.0.0",
      "AdvertiseAddress": consumer_host,
  })
  wan = wan_io.open("interferometer", adios2.Mode.Read)

  bp_io = adios.declare_io("BPOutput")
  bp_io.set_engine("BP5")
  bp = bp_io.open(output_path, adios2.Mode.Write)

  while wan.begin_step() == adios2.StepStatus.OK:
      bp.begin_step()
      for name in wan_io.available_variables():
          source = wan_io.inquire_variable(name)
          data = wan.get(source, adios2.Mode.Sync)
          destination = define_or_update_bp_variable(bp_io, name, data)
          bp.put(destination, data)
      bp.end_step()       # successful downstream EndStep first
      wan.end_step()      # checkpoint and acknowledge second

The order of the last two calls is mandatory. If the BP5 write raises an
exception, do not call WANStreamer ``EndStep()``. The producer retains that
step and retries it after the consumer is repaired or relaunched.

Producer
--------

The producer looks like any other ADIOS2 output option::

  io = adios.declare_io("InterferometerOutput")
  io.set_engine("WANStreamer")
  io.set_parameters({
      "ConnectionFile": connection_file,
      "PrivateKey": private_key,
      "LaunchMode": "none",
      "ReplayBufferSteps": "200",
  })
  stream = io.open("interferometer", adios2.Mode.Write)

  stream.begin_step()
  stream.put(signal_variable, signal)
  stream.put(shot_variable, shot_index)
  stream.end_step()       # copied into WANStreamer's native replay queue

System launch mode
------------------

The writer can launch the consumer through a trusted external command::

  io.set_parameters({
      "ConnectionFile": "/shared/run/consumer-{rank}.json",
      "PrivateKey": "/secure/producer.key",
      "LaunchMode": "system",
      "LaunchCommand":
          "ssh consumer.example python -m streamer.wanstreamer_consumer "
          "{connection_file} /data/raw --public-key /secure/producer.pub "
          "--advertise-host consumer.example",
  })

The command is site policy, not part of the wire protocol. It can instead
submit a scheduler job or invoke a local supervisor. WANStreamer first tries
to reconnect to the existing reader listener. After repeated failures it
runs the launch command again, then continues polling the encrypted
rendezvous. Commands must be treated as trusted configuration because
``LaunchMode=system`` uses the system shell.

Failure behavior
----------------

On a disconnect while steps are queued:

1. The writer keeps the serialized step in its replay queue.
2. It reconnects and repeats key authentication.
3. The reader reports its persistent highest committed step.
4. The writer discards an already committed replay or retransmits an
   uncommitted one.
5. The reader suppresses duplicates and exposes only the next step.
6. The sender removes the step only after the matching acknowledgement.

Writer ``EndStep()`` does not wait for that acknowledgement. It copies the
step into the bounded native queue and returns so acquisition can continue.
When ``ReplayBufferSteps`` or ``ReplayBufferBytes`` is reached, ``EndStep()``
blocks until acknowledgements free space; it never drops a queued step.
``Close()`` drains the queue. The queue is memory-resident, so producer process
failure is outside this guarantee.

The consumer needs no second transport ring buffer. WANStreamer keeps the
current received step until the consumer calls its reader ``EndStep()``. In
the interferometer application, the original socket consumer remains
available; ``streamer.wanstreamer_consumer`` is a separate, additive consumer
that performs only WANStreamer read, BP5 write, then WANStreamer ``EndStep()``.

For MPI, use one producer/consumer connection per rank. ``{rank}`` in the
connection and launch paths keeps rendezvous files separate. WANStreamer does
not aggregate or redistribute rank data.
