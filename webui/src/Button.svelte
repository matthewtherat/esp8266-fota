<script>
import { createEventDispatcher } from 'svelte';
export let title = '';
export let icon;
const dispatch = createEventDispatcher();

function click() {
	dispatch('click', {
		text: 'Hello!'
	});
}

</script>
<style type="text/sass" global>
@import 'styles/global.sass'

button
  float: left
  padding: 6px
  height: 30px
  line-height: 16px
  vertical-align: middle
  svg
    display: block
    float: left
    margin-top: -2px
    width: 18px
    height: 18px
  span
    margin-left: $gutter

</style> 
<button on:click={click}>
  <svg><use xlink:href={"#icon-" + icon}></use></svg>
  {#if title}
    <span>{title}</span>
  {/if}
</button>
