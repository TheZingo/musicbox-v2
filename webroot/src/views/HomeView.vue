<script setup lang="ts">
import type { TagData } from '@/types/TagData';
import MusicBox from '../components/MusicBox.vue'
import TagDataComponent from '../components/TagDataComponent.vue'
import { store } from '../store'
</script>

<template>
    <MusicBox :hasTag="store.hasTag"/> 
    <TagDataComponent :tagData="store.currentTag"/>
</template>