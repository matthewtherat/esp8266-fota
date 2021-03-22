<script context="module">
// main navigation for the application
import Home from './Index.svelte';
import Wifi from './Wifi.svelte';

// these are our 'pages' to navigate through (in order for this export to work the script tag MUST include context="module")
export const navOptions = [
    { title: 'Home',        component: Home,       icon: 'home'       },
    { title: 'Wifi',        component: Wifi,       icon: 'connection' },

		// other navigation pages can go here
];
</script>
