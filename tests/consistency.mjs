// Keeps the places that list the libraries equal: the workflow's checkouts (the pins), the README's pin table and
// intro, CMakeLists.txt's demo targets, and the page's cards, jump links, scripts and repository links. A library
// added or repinned in one place and forgotten in another fails here instead of shipping a stale page.
// Usage: node tests/consistency.mjs [repository root]
import { readFile } from 'node:fs/promises';
import path from 'node:path';

const root = path.resolve(process.argv[2] || '.');
const read = file => readFile(path.join(root, file), 'utf8');
const [workflow, readme, cmake, page] = await Promise.all(['.github/workflows/deploy.yml', 'README.md', 'CMakeLists.txt', 'index.html'].map(read));
const problems = [];
const expect = (condition, message) => { if (!condition) problems.push(message); };

// The pins: every checkout of a dasmig repository, with its ref.
const pins = new Map([...workflow.matchAll(/repository: dasmig\/(\S+)\s+ref: (\S+)/g)].map(match => [match[1], match[2]]));
expect(pins.size > 0, 'deploy.yml checks out no library');
const libraries = [...pins.keys()].filter(name => name !== 'dasmig-core');

// The README's pin table: a row may list several libraries that share a tag.
const tableTags = new Map();
for (const row of readme.matchAll(/^\|\s*([a-z-]+(?:, [a-z-]+)*)\s*\|\s*(v\d[\w.]*)\s*\|$/gm))
  for (const name of row[1].split(', ')) tableTags.set(name, row[2]);
for (const [name, tag] of pins)
  expect(name === 'dasmig-core' || tableTags.get(name) === tag,
    `README pin table has ${name} at ${tableTags.get(name) ?? 'nothing'}, deploy.yml at ${tag}`);
for (const name of tableTags.keys()) expect(pins.has(name), `README pin table lists ${name}, which deploy.yml does not check out`);

// Every library is introduced in the README and linked from a card.
for (const name of libraries) {
  expect(readme.includes(`](https://github.com/dasmig/${name})`), `README intro does not link ${name}`);
  expect(page.includes(`href="https://github.com/dasmig/${name}"`), `index.html has no card linking ${name}`);
}

// Every demo target has a card, a jump link, a script tag and a README mention.
const demos = [...cmake.matchAll(/add_executable\((\w+)_demo /g)].map(match => match[1]);
expect(demos.length === libraries.length, `CMakeLists.txt builds ${demos.length} demos for ${libraries.length} libraries`);
for (const demo of demos) {
  expect(page.includes(`id="${demo}-card"`), `index.html has no card ${demo}-card`);
  expect(page.includes(`href="#${demo}-card"`), `index.html has no jump link to ${demo}-card`);
  expect(page.includes(`<script src="${demo}_demo.js"></script>`), `index.html does not load ${demo}_demo.js`);
  expect(readme.includes(`\`${demo}_demo\``), `README does not list ${demo}_demo`);
}

// Every card is reachable from the jump bar.
for (const match of page.matchAll(/<div class="demo-card" id="([\w-]+)"/g))
  expect(page.includes(`href="#${match[1]}"`), `index.html has no jump link to ${match[1]}`);

if (problems.length > 0) {
  console.error(problems.map(problem => 'FAIL: ' + problem).join('\n'));
  process.exit(1);
}
console.log(`PASS: ${libraries.length} libraries agree across deploy.yml, README.md, CMakeLists.txt and index.html.`);
