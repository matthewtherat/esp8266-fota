<script>
import { navOptions } from  './Nav.svelte';
import Icons from './Icons.svelte';

let selected = navOptions[0];
let intSelected = 0;

function changeComponent(event) {
  let i = event.srcElement.id;
	selected = navOptions[i];
	intSelected = i;
}
</script>

<style type="text/sass" global>
  @import 'styles/global.sass'
  nav svg 
    height: 60px
    cursor: pointer;
    fill: $black;
    &.active
      fill: $orange
    use
      pointer-events: none;
</style>

<Icons />
<div class="smc mdc">
	<!--app navigation -->
	<nav class="row">
		{#each navOptions as n, i}
      <svg id={i} class={intSelected==i ? "active md1" : "md1"} 
           on:click={changeComponent}>
        <use xlink:href={"#icon-" + n.icon}></use>
      </svg>

		{/each}
	</nav>
	<!-- content wrapper -->
	<div class="row">
		<div class="col-sm-12">
			<div class="p-2">
				<h1>{selected.title}</h1>
				<!-- this is where our main content is placed -->
				<svelte:component this={selected.component}/>
			</div>
		</div>
	</div>
</div>
