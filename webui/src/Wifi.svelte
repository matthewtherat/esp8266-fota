<script>
import { onMount } from 'svelte';
let disabled = true;
let p = {};

onMount(async () => {
  //document.title = title;
  const res = await fetch(`/params.json`);
  if (res.ok) {
    p = await res.json();
    //title = document.title = `${p.zone}.${p.name}`;
    disabled = false;
  }
  else {
    throw new Error(res.text());
  }
});

async function submit(event) {
  var formraw = {
    'zone':     p.zone,
    'name':     p.name,
    'ap_psk':   p.apPsk,
    'ssid':     p.ssid,
    'psk':      p.psk
  };

  var form = [];
  for (var property in formraw) {
    var encodedKey = encodeURIComponent(property);
    var encodedValue = encodeURIComponent(formraw[property]);
    form.push(encodedKey + "=" + encodedValue);
  }
  form = form.join("&");

  const res = await fetch('/params', {
    method: 'POST',
    headers: {
    'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8'
    },
    body: form
  });

}
</script>

<h4>WIFI Parameters</h4>
<hr>

<p style="display: {disabled? 'block': 'none'}" >
  Cannot load params.
</p>
<div class="row" style="display: {disabled? 'none': 'block'}">
  <form>
  <div class="row">
    <div class="lg4">Zone:</div>
    <div class="lg6">
      <input name="zone" bind:value="{p.zone}" {disabled}/>
    </div>
  </div>
  <div class="row">
    <div class="lg4">Name:</div>
    <div class="lg6">
      <input name="name" bind:value="{p.name}" {disabled}/>    
    </div>
  </div>
  <div class="row">
    <div class="lg4">Access Point PSK</div>
    <div class="lg6">
      <input name="ap_psk" bind:value="{p.apPsk}" {disabled}/>
    </div>
  </div>
  <div class="row">
    <div class="lg4">SSID</div>
    <div class="lg6">
      <input name="ssid" bind:value="{p.ssid}" {disabled}/>    
    </div>
  </div>
  <div class="row">
    <div class="lg4">PSK</div>
    <div class="lg6">
      <input name="psk" bind:value="{p.psk}" {disabled}/>    
    </div>
  </div>
  <div class="row">
    <div class="lg4"></div>
    <div class="lg6">
      <button type="button" {disabled} on:click={submit} >
        Save & Reboot
      </button>
    </div>
  </div>
  </form> 
</div>
