<script lang="ts">
	import type { Action } from "svelte/action";

	let counter = $state(1);

	const action: Action = () => {
		$effect(() => {
			const handle = setInterval(() => {
				counter++;
			}, 1000);

			return () => clearInterval(handle);
		});
	};
</script>


<div use:action class="flex flex-col items-center justify-center w-[400px] h-[60px] bg-transparent">
	<p class="text-2xl font-bold text-center text-red-500 hover:text-blue-500">Hello, world! Counter: {counter}</p>
</div>
