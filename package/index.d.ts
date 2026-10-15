// Types for the dasmig persona generator. The JSON shapes are those of the
// C++ libraries' to_json(); see each library's doc/usage.md.

/** What a persona call asks for. Every field is optional: what is left out is drawn. */
export interface PersonaRequest {
  /** ISO 3166-1 alpha-2 code, any case. Unset draws one of `countries`, weighted by population. */
  country?: string;
  /** Unset draws it with the birth. */
  sex?: Sex;
  /** Inclusive age range in completed years (0-120). Unset draws from the country's age pyramid. */
  ages?: { min: number; max: number };
  /** Number of personality traits; default 3. */
  traits?: number;
  /**
   * The date ages are computed at: `YYYY-MM-DD`, or a Date (its local calendar date).
   * Default 2023-07-01. The generator never reads the clock, so pass today's date for ages "as of now".
   */
  referenceDate?: string | Date;
  /**
   * The call seed: 1-16 hexadecimal digits, a bigint, or a non-negative safe integer.
   * Unset draws a random one. The same seed, request and data give the same persona.
   */
  seed?: string | bigint | number;
}

export type Sex = 'male' | 'female';

/** The seed a result was generated from and a hash of the data it used (16 hex digits each). */
export interface Signature {
  seed: string;
  data: string;
}

export interface Country {
  cca2: string;
  cca3: string;
  ccn3: string;
  name_common: string;
  name_official: string;
  capital: string[];
  region: string;
  subregion: string;
  continent: string[];
  latitude: number;
  longitude: number;
  area: number;
  population: number;
  landlocked: boolean;
  independent: boolean;
  un_member: boolean;
  languages: string[];
  currency_code: string;
  currency_name: string;
  currency_symbol: string;
  borders: string[];
  timezones: string[];
  driving_side: string;
  tld: string;
  idd_root: string;
  idd_suffix: string;
  demonym_m: string;
  demonym_f: string;
  flag_emoji: string;
  income_level: string;
  start_of_week: string;
  signature: Signature;
}

export interface City {
  geonameid: number;
  name: string;
  asciiname: string;
  latitude: number;
  longitude: number;
  feature_code: string;
  country_code: string;
  cc2: string;
  admin1_code: string;
  admin2_code: string;
  admin3_code: string;
  admin4_code: string;
  population: number;
  /** Metres; null when unknown. */
  elevation: number | null;
  /** Digital elevation model, metres. */
  dem: number;
  timezone: string;
  signature: Signature;
}

export interface Birth {
  country_code: string;
  /** YYYY-MM-DD. */
  date: string;
  year: number;
  month: number;
  day: number;
  /** Completed years at the reference date. */
  age: number;
  sex: Sex;
  /** 0 = Sunday. */
  weekday: number;
  /** Remaining life expectancy at that age, years (a population model, not a forecast). */
  le_remaining: number;
  cohort: string;
  signature: Signature;
}

export interface Biodata {
  country: string;
  sex: Sex;
  height_cm: number;
  weight_kg: number;
  bmi: number;
  eyes: string;
  hair: string;
  /** Fitzpatrick type, I to VI. */
  skin: string;
  blood: string;
  hand: string;
  age: number;
  signature: Signature;
}

export interface Trait {
  key: string;
  label: string;
  /** The exclusive group the trait belongs to, if any. */
  group: string | null;
  tags: string[];
}

export interface Personality {
  traits: Trait[];
  /** Big Five scores: centered on zero, unbounded model values, not percentiles. */
  axes: {
    openness: number;
    conscientiousness: number;
    extraversion: number;
    agreeableness: number;
    neuroticism: number;
  };
  signature: Signature;
}

export interface InterestItem {
  key: string;
  label: string;
}

export interface Interest {
  country: string;
  religion: string;
  religion_group: string;
  hobbies: InterestItem[];
  films: InterestItem[];
  games: InterestItem[];
  books: InterestItem[];
  music: InterestItem[];
  signature: Signature;
}

export type EmploymentStatus = 'employed' | 'unemployed' | 'student' | 'retired' | 'inactive';
export type EducationLevel = 'less_than_basic' | 'basic' | 'intermediate' | 'advanced';

export interface Job {
  key: string;
  label: string;
  /** ISCO-08 major or sub-major group code. */
  isco: string;
  isco_label: string;
  /** ISCO-08 unit group code. */
  unit: string;
  /** ISCO-08 skill level, "1" to "4". */
  skill: string;
  skills: string[];
  gear: string[];
  tags: string[];
}

export interface Occupation {
  country: string;
  sex: Sex;
  age: number;
  status: EmploymentStatus;
  education: EducationLevel | null;
  /** Null unless employed. */
  job: Job | null;
  signature: Signature;
}

/** One persona: each library's result, as that library writes it. */
export interface Persona {
  name: string;
  sex: Sex;
  age: number;
  /** The city's state or province. */
  admin1: string;
  country: Country;
  city: City;
  birth: Birth;
  biodata: Biodata;
  personality: Personality;
  interest: Interest;
  occupation: Occupation;
  /** `generate({ ...request, seed: persona.signature.seed })` replays this persona. */
  signature: Signature;
}

export interface PersonaGenerator {
  /** The countries a random persona is drawn from (ISO 3166-1 alpha-2, sorted). */
  readonly countries: readonly string[];
  /** Hash of the data of all eight libraries, as in every persona's `signature.data`. */
  readonly dataHash: string;
  /** Generate a persona. Throws on an invalid request (for example an unknown country). */
  generate(request?: PersonaRequest): Persona;
  /** Generate a persona as the JSON text the module returns. */
  generateJson(request?: PersonaRequest): string;
}

/**
 * Load the WebAssembly module and return a generator.
 * @param moduleOptions Emscripten module options, for example `locateFile` when
 *   persona.wasm is served from another path than the module.
 */
export function createPersonaGenerator(moduleOptions?: Record<string, unknown>): Promise<PersonaGenerator>;
