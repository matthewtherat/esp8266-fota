<script>
import { onMount } from 'svelte';
export let title;
let disabled = true;
let p = {};

onMount(async () => {
  document.title = title;
  const res = await fetch(`/params.json`);
  if (res.ok) {
    p = await res.json();
    title = document.title = `${p.zone}.${p.name}`;
    disabled = false;
  }
  else {
    throw new Error(res.text());
  }
});

</script>

<style type="text/sass" global>
  body
    margin: 0px;
  
  input
    margin: 6px;
  
  h1
    color: blue
</style>

<h1>{title}</h1>
<div class="p">
  <p style="display: {disabled? 'block': 'none'}" >
    Cannot load params.
  </p>
  <form action="/params" method="post" 
    style="display: {disabled? 'none': 'block'}">

    <h4>Settings</h4>
    zone: <input name="zone" bind:value="{p.zone}" {disabled}/><br/>
    name: <input name="name" bind:value="{p.name}" {disabled}/><br/>
    AP PSK: <input name="ap_psk" bind:value="{p.apPsk}" {disabled}/><br/>
    SSID: <input name="ssid" bind:value="{p.ssid}" {disabled}/><br/> 
    PSK: <input name="psk" bind:value="{p.psk}" {disabled}/><br/>
    <input type="submit" value="Reboot" {disabled} />
  </form> 
</div>
