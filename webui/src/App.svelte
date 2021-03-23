<script>
import { routes } from  './Nav.svelte';
import Icons from './Icons.svelte';

export let title;

let selectedIndex = routes.findIndex(e => e.path == window.location.pathname)
let selected = routes[selectedIndex];

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
#header
  >div
    height: 200px

#middle
  >div:first-child
    padding-left: $gutter * 6
  min-height: 100%    

#comp, #hcomp
  >div:nth-child(2)
    background: $bgdark
    height: 100%
    min-height: 100%

#hcomp
  line-height: 200px
  svg
    height: 100px
    margin-top: 35px
    margin-bottom: 20px
    padding-right: $gutter * 4
  h2
    padding: 0px 0px 0px $gutter * 5
  >div:nth-child(2)
    border-top-left-radius: 19px

#comp
  height: 100%
  min-height: 100%
  >div:nth-child(2)
    padding: $gutter * 6

nav 
  a 
    display: block
    height: 40px
    line-height: 40px - $gutter * 2
    margin-bottom: $gutter * 2
    vertical-align: middle
    cursor: pointer
    *
      pointer-events: none
    svg 
      display: block
      width: 100%
      height: 100%
    &.active
      svg 
        fill: $orange
    &:hover
      background: $bgdark
</style>

<Icons />
<!-- Header -->
<div id="header" class="row">
  <div class="lg2"></div>
  <div class="lg2"></div>
  <div id="hcomp" class="lg5">
    <div class="lg1"></div>
    <div class="lg9">
      <h2 class="lg8">{selected.title}</h2>
      <svg class="lg2"><use xlink:href={"#icon-" + selected.icon}></use></svg>
    </div>
  </div>
  <div class="lg1"></div>
</div>
<div id="middle" class="row">
  <div class="lg2">
    <h1>{title}</h1>
  </div>
  <!-- App navigation -->
  <nav class="lg2">
  	{#each routes as n, i}
      <a title={n.title} id={i} 
         class={selectedIndex==i ? "active row" : "row"} 
         on:click={changeComponent}>
        <svg class="lg2"><use xlink:href={"#icon-" + n.icon}></use></svg>
        <h6 class="lg8">{n.title}</h6>
      </a>
  	{/each}
  </nav>
  <div id="comp" class="lg5">
    <div class="lg1"></div>
    <div class="lg9">
		  <!-- this is where our main content is placed -->
		  <svelte:component this={selected.component}/>
    </div>
  </div>
  <div class="lg1"></div>
</div>

