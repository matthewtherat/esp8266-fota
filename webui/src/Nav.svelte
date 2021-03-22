<script context="module">
// main navigation for the application
import Home from './Index.svelte';
import About from './About.svelte';
import Wifi from './Wifi.svelte';
import SourceCode from './SourceCode.svelte';
import Terminal from './Terminal.svelte';
import Reboot from './Reboot.svelte';

// these are our 'pages' to navigate through (in order for this export to work the script tag MUST include context="module")
export const navOptions = [
    { title: 'Home',        component: Home,       icon: 'home'       },
    { title: 'Wifi',        component: Wifi,       icon: 'connection' },
    { title: 'Reboot',      component: Reboot,     icon: 'switch'     },
    { title: 'Source code', component: SourceCode, icon: 'github'     },
    { title: 'Terminal',    component: Terminal,   icon: 'terminal'   },
		{ title: 'About',       component: About,      icon: 'question'   },

		// other navigation pages can go here
];
</script>
