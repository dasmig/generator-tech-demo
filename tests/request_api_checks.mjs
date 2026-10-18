import assert from 'node:assert/strict';

// `call` invokes the actual demo binding and parses its returned JSON.
export async function verifyRequestApis(call) {
  const cases = [
    ['ngMod', 'generateNameSeeded', ['female', 'br', 'full']],
    ['nngMod', 'generateNicknameSeeded', ['José García']],
    ['ctgMod', 'sampleCountrySeeded', ['Europe', true]],
    ['cgMod', 'sampleCitySeeded', ['BR', false]],
    ['bgMod', 'generateBirthSeeded', ['US', 'female', '2024-02-29', -1, 20, 25]],
    ['bdMod', 'generateBiodataSeeded', ['BR', 'male', -1]],
    ['igMod', 'generateInterestSeeded', ['US', 'female', 30, 2, 1]],
    ['ogMod', 'generateOccupationSeeded', ['DE', 'male', 35, 'advanced', 'employed']],
    ['phMod', 'generatePhoneSeeded', ['US', 'mobile', false]],
    ['emMod', 'generateEmailSeeded', ['DE', 'Zoë', 'Müller', 'zm', 1988, false]]
  ];
  for (const seed of ['0', '1', 'ffffffffffffffff']) {
    for (const [module, method, args] of cases) {
      const result = await call(module, method, [...args, seed]);
      assert.equal(result.error, undefined, `${module}.${method}: ${result.error}`);
      assert.equal(result.signature.seed, seed.padStart(16, '0'));
      // Interleave different requests and seeds before replaying the original.
      await call(module, method, [...args, seed === '1' ? '2' : '1']);
      assert.deepEqual(await call(module, method, [...args, result.signature.seed]), result);
    }
    const traits = await call('tgMod', 'generateTraits', [3, seed]);
    assert.deepEqual(await call('tgMod', 'generateTraits', [3, traits.signature.seed]), traits);
    const entity = await call('egMod', 'generateEntitySeeded', [seed, false]);
    assert.equal(entity.signature.seed, seed.padStart(16, '0'));
    const subset = await call('egMod', 'generateEntitySeeded', [seed, true]);
    assert.deepEqual(Object.keys(subset.components), ['race', 'class', 'alignment']);
    for (const key of Object.keys(subset.components)) assert.equal(subset.components[key], entity.components[key]);
    assert.deepEqual(await call('egMod', 'generateEntitySeeded', [entity.signature.seed, false]), entity);
    assert.equal(Object.keys(entity.components).length, 10);
    assert.ok(entity.components.level >= 1 && entity.components.level <= 20);
    for (const key of ['strength', 'dexterity', 'constitution', 'intelligence', 'wisdom', 'charisma'])
      assert.ok(entity.components[key] >= 3 && entity.components[key] <= 18);

    for (const country of ['', 'BR', 'JP']) {
      const args = [country, '', '2024-02-29', seed];
      const persona = await call('pgMod', 'generatePersonaSeeded', args);
      assert.equal(persona.error, undefined, persona.error);
      assert.equal(persona.signature.seed, seed.padStart(16, '0'));
      if (country) assert.equal(persona.country.cca2, country);
      assert.ok(persona.name && persona.city.name && persona.biodata.height_cm && persona.interest.religion
        && persona.occupation.status && persona.personality.traits.length === 3);
      assert.equal(persona.biodata.age, persona.age, 'persona-generator passes the age to biodata');
      const [year, month, day] = persona.birth.date.split('-').map(Number);
      assert.equal(persona.age, 2024 - year - Number(month > 2 || (month === 2 && day > 29)));
      await call('pgMod', 'generatePersonaSeeded', ['US', 'female', '2023-01-01', 'abcd']);
      await call('pgMod', 'generatePersona', ['DE', '']);
      assert.deepEqual(await call('pgMod', 'generatePersonaSeeded', args), persona,
        'A persona must replay from its seed, not from singleton entropy');
      // Forcing the sex changes the parts that depend on it, not the others.
      const other = persona.sex === 'female' ? 'male' : 'female';
      const forced = await call('pgMod', 'generatePersonaSeeded', [persona.country.cca2, other, '2024-02-29', seed]);
      assert.equal(forced.sex, other);
      assert.deepEqual([forced.city, forced.personality], [persona.city, persona.personality]);
    }
  }

  for (const mode of ['full', 'first', 'surname']) {
    const name = await call('ngMod', 'generateNameSeeded', ['female', 'br', mode, '1']);
    assert.equal(name.culture, 'brazilian');
    assert.equal(name.parts.length, mode === 'full' ? 2 : 1);
    if (mode !== 'surname') assert.equal(name.gender, 'female');
    for (const count of [1, 5, 10]) {
      const alternatives = await call('ngMod', 'generateNameOptions', ['female', 'br', mode, count]);
      assert.equal(alternatives.length, count);
      for (const alternative of alternatives)
        assert.deepEqual(await call('ngMod', 'generateNameSeeded',
          ['female', 'br', mode, alternative.signature.seed]), alternative);
    }
  }
  for (const input of ['', 'José García']) {
    const nickname = await call('nngMod', 'generateNicknameSeeded', [input, 'ffffffffffffffff']);
    assert.ok(nickname.nickname && nickname.source);
    const alternatives = await call('nngMod', 'generateNicknameOptions', [input, 5]);
    for (const alternative of alternatives)
      assert.deepEqual(await call('nngMod', 'generateNicknameSeeded', [input, alternative.signature.seed]), alternative);
  }
  for (const weighted of [true, false]) {
    const country = await call('ctgMod', 'sampleCountrySeeded', ['Europe', weighted, '1']);
    assert.equal(country.region, 'Europe');
    await call('ctgMod', 'sampleCountrySeeded', ['Asia', !weighted, '1']);
    await call('ctgMod', 'generateCountry', ['Africa']);
    assert.deepEqual(await call('ctgMod', 'sampleCountrySeeded', ['Europe', weighted, '1']), country);
    const city = await call('cgMod', 'sampleCitySeeded', ['BR', weighted, '1']);
    assert.equal(city.country_code, 'BR');
    await call('cgMod', 'sampleCitySeeded', ['US', !weighted, '1']);
    await call('cgMod', 'generateCity', ['JP']);
    assert.deepEqual(await call('cgMod', 'sampleCitySeeded', ['BR', weighted, '1']), city);
  }
  assert.ok((await call('ctgMod', 'sampleCountrySeeded', ['', true, '1'])).cca2);
  assert.ok((await call('cgMod', 'sampleCitySeeded', ['', false, '1'])).geonameid);
  for (const seed of ['0', '1', 'ffffffffffffffff']) {
    const birth = await call('bgMod', 'generateBirthSeeded', ['US', 'female', '2024-02-29', -1, 20, 25, seed]);
    assert.equal(birth.country_code, 'US');
    assert.equal(birth.sex, 'female');
    assert.ok(birth.age >= 20 && birth.age <= 25);
    const fixed = await call('bgMod', 'generateBirthSeeded', ['BR', 'male', '2024-02-29', 1990, -1, -1, seed]);
    assert.equal(fixed.year, 1990);
    assert.equal(fixed.country_code, 'BR');
    assert.equal(fixed.sex, 'male');
    const base = await call('bdMod', 'generateBiodataSeeded', ['BR', 'male', -1, seed]);
    const child = await call('bdMod', 'generateBiodataSeeded', ['BR', 'male', 8, seed]);
    assert.equal(Object.hasOwn(base, 'age'), false);
    assert.equal(child.age, 8);
    assert.equal(child.country, 'BR');
    assert.equal(child.sex, 'male');
    assert.ok(child.height_cm < base.height_cm && child.weight_kg < base.weight_kg);
    for (const key of ['eyes', 'skin', 'blood', 'hand']) assert.equal(child[key], base[key]);
    assert.deepEqual(await call('bdMod', 'generateBiodataSeeded', ['BR', 'male', -1, seed]), base);
  }
  for (const [age, hobbies, media] of [[-1, 3, 2], [30, 2, 1], [8, 0, 0]]) {
    const interests = await call('igMod', 'generateInterestSeeded', ['US', '', age, hobbies, media, '1']);
    assert.equal(interests.country, 'US');
    assert.equal(interests.hobbies.length, hobbies);
    for (const key of ['films', 'games', 'books', 'music']) assert.equal(interests[key].length, media);
  }
  for (const status of ['employed', 'unemployed', 'student', 'retired', 'inactive']) {
    const occupation = await call('ogMod', 'generateOccupationSeeded', ['DE', 'female', 35, 'advanced', status, '1']);
    assert.equal(occupation.country, 'DE');
    assert.equal(occupation.sex, 'female');
    assert.equal(occupation.age, 35);
    assert.equal(occupation.education, 'advanced');
    assert.equal(occupation.status, status);
    if (status === 'employed') assert.ok(occupation.job);
    if (status === 'student') assert.equal(occupation.job, null);
  }
  const childJob = await call('ogMod', 'generateOccupationSeeded', ['BR', 'female', 8, '', '', '1']);
  assert.equal(childJob.status, 'student');
  assert.equal(childJob.job, null);
  for (const country of ['', 'US', 'BR']) {
    for (const biodata of await call('bdMod', 'compareBiodata', [country]))
      assert.deepEqual(await call('bdMod', 'generateBiodataSeeded',
        [biodata.country, biodata.sex, -1, biodata.signature.seed]), biodata);
    for (const sex of ['', 'male', 'female']) {
      for (const { age, interest } of await call('igMod', 'compareAges', [country, sex]))
        assert.deepEqual(await call('igMod', 'generateInterestSeeded',
          [interest.country, sex, age, 3, 2, interest.signature.seed]), interest);
      for (const { education, occupation } of await call('ogMod', 'compareEducation', [country, sex]))
        assert.deepEqual(await call('ogMod', 'generateOccupationSeeded',
          [occupation.country, occupation.sex, 35, education, 'employed', occupation.signature.seed]), occupation);
    }
  }
  const errors = [
    ['ctgMod', 'sampleCountrySeeded', ['invalid-region', true, '1']],
    ['cgMod', 'sampleCitySeeded', ['XX', false, '1']],
    ['bgMod', 'generateBirthSeeded', ['US', '', '2024-02-29', 1990, 20, 25, '1']],
    ['bdMod', 'generateBiodataSeeded', ['US', '', 121, '1']],
    ['igMod', 'generateInterestSeeded', ['XX', '', 30, 2, 1, '1']],
    ['ogMod', 'generateOccupationSeeded', ['US', '', 35, 'unknown', '', '1']],
    ['ogMod', 'generateOccupationSeeded', ['BR', 'female', 8, '', 'employed', '1']],
    ['phMod', 'generatePhoneSeeded', ['XX', '', false, '1']],
    ['phMod', 'generatePhoneSeeded', ['US', 'cell', false, '1']],
    ['emMod', 'generateEmailSeeded', ['XX', '', '', '', 0, false, '1']],
    ['pgMod', 'generatePersonaSeeded', ['XX', '', '2024-02-29', '1']],
    ['pgMod', 'generatePersonaSeeded', ['US', 'other', '2024-02-29', '1']],
    ['pgMod', 'generatePersonaSeeded', ['US', '', '2024-02-29', '10000000000000000']]
  ];
  for (const [module, method, args] of errors) assert.ok((await call(module, method, args)).error);
  for (const [module, method, args] of cases) {
    assert.match((await call(module, method, [...args, '10000000000000000'])).error, /hexadecimal/);
  }
}
