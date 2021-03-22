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
#index .row
    line-height: 28px

#index .row.h
  margin-top: $gutter * 5 !important
  padding-left: 0px
  svg
    height: 51px
    padding-left: 0px
  h4
    line-height: 50px - $gutter * 2

#index hr
  margin-top: 0px

#index 
  button svg
    width: 20px
    height: 20px
</style>

<div id="index">
  <div class="row h">
    <svg class="lg1"><use xlink:href="#icon-spinner2"></use></svg>
    <h4 class="lg9">System Status</h4>
  </div>
  <hr />
  <div class="row">
    <div class="lg4">Name:</div>
    <div class="lg4">{title}</div>
  </div>
  <div class="row">
    <div class="lg4">Uptime:</div>
    <div class="lg4">{p.uptime}</div>
    <div class="lg2">
      <button on:click={reboot}>
        <svg><use xlink:href="#icon-switch"></use></svg>
      </button>
    </div>
  </div>
  <div class="row">
    <div class="lg4">Boot:</div>
    <div class="lg4">{p.boot}</div>
    <div class="lg2">
      <button on:click={toggleBoot}>
        <svg><use xlink:href="#icon-loop2"></use></svg>
      </button>
    </div>
  </div>
  <div class="row">
    <div class="lg4">Version:</div>
    <div class="lg4">{p.version}</div>
  </div>
  <div class="row">
    <div class="lg4">Free Memory:</div>
    <div class="lg4">{p.free} Bytes</div>
  </div>
  <div class="row">
    <div class="lg4">RTC Clock:</div>
    <div class="lg4">{p.rtc}</div>
  </div>

  <div class="row h">
    <svg class="lg1"><use xlink:href="#icon-github"></use></svg>
    <h4 class="lg9">Source Code</h4>
  </div>
  <hr>
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

  <div class="row h">
    <svg class="lg1"><use xlink:href="#icon-bug"></use></svg>
    <h4 class="lg9">Bug Report</h4>
  </div>
  <hr>
  <p>
  Visit 
  <a href="https://github.com/pylover/esp8266-fota/issues">here</a>
  to submit any issue.
  </p>

  <div class="row h">
    <svg class="lg1"><use xlink:href="#icon-terminal"></use></svg>
    <h4 class="lg9">Command Line Interface</h4>
  </div>
  <hr>
  <p>
  You may install 
  <a href="https://github.com/pylover/unspy">unspy</a>
  to control the device via CLI.
  </p>


</div>


