// A pool worker for Node (a worker thread started by createPersonaPool).
import { parentPort } from 'node:worker_threads';
import { createPersonaGenerator } from './node.mjs';
import { serve } from './pool-worker-core.mjs';

serve(await createPersonaGenerator(), message => parentPort.postMessage(message), handler => {
  parentPort.on('message', handler);
});
