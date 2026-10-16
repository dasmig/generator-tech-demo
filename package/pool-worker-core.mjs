// What a pool worker does once it has a generator: answer batch pieces. The
// two worker entry points (pool-worker.mjs for browsers, pool-worker-node.mjs
// for Node) supply the generator and the messaging.
export function serve(generator, send, subscribe) {
  subscribe(message => {
    try {
      send({ id: message.id, jsonl: generator.generateBatchJsonl(message.options) });
    } catch (error) {
      send({ id: message.id, error: { name: error.name, message: error.message } });
    }
  });
  send({ ready: true });
}
