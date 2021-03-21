<script>
import { navOptions } from  './Nav.svelte';
import Icons from './Icons.svelte';

export let title;
let selected = navOptions[0];
let intSelected = 0;

function changeComponent(event) {
  let i = event.srcElement.id;
  console.log(event);
	selected = navOptions[i];
	intSelected = i;
}
</script>

<style type="text/sass" global>
@import 'styles/global.sass'
#header
  padding: $gutter * 3

nav 
  float: $float
  padding-left: $gutter * 3
  a 
    display: block
    height: 40px
    line-height: 40px
    vertical-align: middle
    cursor: pointer
    *
      pointer-events: none
    h4
      bottom: 0px
      padding-left: $gutter
    svg 
      display: block
      width: 100%
      height: 100%
      fill: $forecolor
    &.active
      svg 
        fill: $orange
</style>

<Icons />
<div id="header" class="row">
  <!-- Header -->
  <div class="all10">
    <h1>{title}</h1>
  </div>
</div>

<div class="row">
  <!-- App navigation -->
  <nav class="lg2">
  	{#each navOptions as n, i}
      <a title={n.title} id={i} 
         class={intSelected==i ? "active row4" : "row4"} 
         on:click={changeComponent}>
        <svg class="lg2"><use xlink:href={"#icon-" + n.icon}></use></svg>
        <h4 class="lg8">{n.title}</h4>
      </a>
  	{/each}
  </nav>

	<!-- Content wrapper -->
	<div class="lg7">
		<div class="row4">
				<h2>{selected.title}</h2>
				<!-- this is where our main content is placed -->
				<svelte:component this={selected.component}/>
		</div>
	</div>
</div>
