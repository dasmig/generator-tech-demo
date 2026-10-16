// A pool worker for browsers (a module Web Worker started by createPersonaPool).
import { createPersonaGenerator } from './index.mjs';
import { serve } from './pool-worker-core.mjs';

serve(await createPersonaGenerator(), message => self.postMessage(message), handler => {
  self.onmessage = event => handler(event.data);
});
