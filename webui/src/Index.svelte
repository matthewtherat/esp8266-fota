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
</script>


<h4>System Status</h4>
<hr>
<div class="row">
  <div class="lg4">Name:</div>
  <div class="lg4">{title}</div>
</div>
<div class="row">
  <div class="lg4">Uptime:</div>
  <div class="lg4">{p.uptime}</div>
</div>
<div class="row">
  <div class="lg4">Boot:</div>
  <div class="lg4">{p.boot}</div>
  <div class="lg2">
    <button on:click={toggleBoot}>Toggle</button>
  </div>
</div>
<div class="row">
  <div class="lg4">Version:</div>
  <div class="lg4">{p.version}</div>
</div>
<div class="row">
  <div class="lg4">Free Memory:</div>
  <div class="lg4">{p.free} KB</div>
</div>
<div class="row">
  <div class="lg4">RTC Clock:</div>
  <div class="lg4">{p.rtc}</div>
</div>
