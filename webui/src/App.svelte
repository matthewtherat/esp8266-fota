<script>
import Icons from './Icons.svelte';
import Home from './Index.svelte';
import Wifi from './Wifi.svelte';

export let title;
const routes = [
    { title: 'Home', component: Home, path: '/',     icon: 'home'       },
    { title: 'Wifi', component: Wifi, path: '/wifi', icon: 'connection' },
];

let selectedIndex = routes.findIndex(e => e.path == window.location.pathname)
let selected = routes[selectedIndex];

export function changeComponent(event) {
  console.log(event);
  let i = event.srcElement.id;
  selected = routes[i];
  selectedIndex = i;
  window.history.pushState({}, 
    selected.title, 
    `${window.location.origin}${selected.path}`
  );
}

</script>

<style type="text/sass" global>
@import 'styles/global.sass'

.nav-item
  display: block
  float: left
  margin-bottom: $gutter * 3
  *
    pointer-events: none
  h5
    line-height: $nav-icon-size
    vertical-align: middle
    padding-left: $gutter
  svg 
    display: block
    height: $nav-icon-size

h1, h2
  line-height: 72px
  vertical-align: middle

.header
  margin-bottom: $gutter

</style>

<Icons />
<!-- Left Bar -->
<div class="lg2">

  <!-- Main Title -->
  <div class="all10 p3 header">
    <h1>{title}</h1>
  </div>

  <!-- App navigation -->
  <nav class="lg10 p3">
      {#each routes as n, i}
        <a title={n.title} id={i} 
           class={selectedIndex==i ? "nav-item active" : "nav-item"} 
           on:click={changeComponent}>
          <svg class="lg2"><use xlink:href={"#icon-" + n.icon}></use></svg>
          <h5 class="lg8">{n.title}</h5>
        </a>
      {/each}
    </nav>
</div>

<!-- Content -->
<div class="lg7">
  <div id="contentHeader" class="all10 p3 header">
    <h2>{selected.title}</h2>
  </div>
  <div id="content" class="all10 p3">
    <!-- this is where our main content is placed -->
    <svelte:component this={selected.component}/>
  </div>
</div>
<div class="lg1"></div>

