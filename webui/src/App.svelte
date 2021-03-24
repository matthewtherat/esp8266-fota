<script>
import Icons from './Icons.svelte';
import Home from './Index.svelte';
import Wifi from './Wifi.svelte';
import Button from './Button.svelte';

export let title;
const routes = [
    { title: 'Home', component: Home, path: '/',     icon: 'home'       },
    { title: 'Wifi', component: Wifi, path: '/wifi', icon: 'connection' },
];

let selectedIndex = routes.findIndex(e => e.path == window.location.pathname)
let selected = routes[selectedIndex];
let menu = false;

function changeComponent(event) {
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
  padding: $gutter
  margin-bottom: $gutter 
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
  line-height: 28px
  vertical-align: middle
  float: left

.header
  margin-bottom: $gutter

#contentHeader button
  margin-right: $gutter * 2

.visible
  display: block

.hide
  display: none !important
</style>

<Icons />
<!-- Left Bar -->
<div class={"xg2 lg2 " + (menu? "visible" : "hide")} >

  <!-- Main Title -->
  <div class="all10 p3 header">
    <h1>{title}</h1>
  </div>

  <!-- App navigation -->
  <nav class="all10 p3">
      {#each routes as n, i}
        <a title={n.title} id={i} 
           class={'nav-item ' + (selectedIndex==i ? 'active' : '')} 
           on:click={changeComponent}>
          <svg class="all2"><use xlink:href={"#icon-" + n.icon}></use></svg>
          <h5 class="all8">{n.title}</h5>
        </a>
      {/each}
    </nav>
</div>

<!-- Content -->
<div class="xg6 lg6">
  <div id="contentHeader" class="all10 p3 header">
    <Button icon="menu" on:click={e => menu = !menu} />
    <h2>{selected.title}</h2>
  </div>
  <div id="content" class="all10 p3">
    <!-- this is where our main content is placed -->
    <svelte:component this={selected.component}/>
  </div>
</div>

