<script>
import { onMount } from 'svelte';

let loading = true;
let title;
let p = {};

onMount(async () => {
  const res = await fetch(`/status.json`);
  if (res.ok) {
    p = await res.json();
    title = `${p.zone}.${p.name}`;
    loading = false;
  }
  else {
    throw new Error(res.text());
  }
});


async function toggleBoot(event) {
  const res = await fetch('/boots', {method: 'TOGGLE'});
}

async function reboot(event) {
  const res = await fetch('/', {method: 'REBOOT'});
}
</script>

<style type="text/sass">
@import 'styles/variables.sass'
#index > .row
  height: $nav-icon-size
  > div:first-child
    padding-right: $gutter * 2
    text-align: right
</style>

<div id="index">
  <h4 class="all10 section">
    <svg class="lg1"><use xlink:href="#icon-spinner2"></use></svg>
    System Status
  </h4>
  <div class="all10 row">
    <div class="lg1">Name:</div>
    <div class="lg2">{title}</div>
  </div>
  <div class="all10 row">
    <div class="lg1">Uptime:</div>
    <div class="lg2">{p.uptime}</div>
    <div class="lg1">
      <a href="" on:click={reboot}>Reboot</a>
    </div>
  </div>
  <div class="all10 row">
    <div class="lg1">Boot:</div>
    <div class="lg2">{p.boot}</div>
    <div class="lg1">
      <a href="" on:click={toggleBoot}>Toggle</a>
    </div>
  </div>
  <div class="all10 row">
    <div class="lg1">Version:</div>
    <div class="lg2">{p.version}</div>
  </div>
  <div class="all10 row">
    <div class="lg1">Free Memory:</div>
    <div class="lg2">{p.free} Bytes</div>
  </div>
  <div class="all10 row">
    <div class="lg1">RTC Clock:</div>
    <div class="lg2">{p.rtc}</div>
  </div>

  <h4 class="all10 section">
    <svg class="lg1"><use xlink:href="#icon-github"></use></svg>
    Source Code
  </h4>
  <p>
   Checkout these repositories to find the source code and figure out how to 
   cook it!
  </p>
  <ul>
    <li>
      <a href="https://github.com/pylover/esp8266-env">
       https://github.com/pylover/esp8266-env</a>
    </li>
    <li>
      <a href="https://github.com/pylover/esp8266-fota">
       https://github.com/pylover/esp8266-fota</a>
    </li>
  </ul>

  <h4 class="all10 section">
    <svg class="lg1"><use xlink:href="#icon-bug"></use></svg>
    Bug Report
  </h4>
  <p>
    Visit 
    <a href="https://github.com/pylover/esp8266-fota/issues">here</a>
    to submit any issue.
  </p>

  <h4 class="all10 section">
    <svg class="lg1"><use xlink:href="#icon-terminal"></use></svg>
    Command Line Interface
  </h4>
  <p>
    You may install 
    <a href="https://github.com/pylover/unspy">unspy</a>
    to control the device via CLI.
  </p>
</div>


