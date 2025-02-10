<script setup lang="ts">
import type { TagData } from '@/types/TagData';
import axios from 'axios';
import { onMounted, ref, type Ref } from 'vue';
import { useRoute } from 'vue-router'
import { formatTimestamp } from '@/dateformat';

const route = useRoute();

const tags: Ref<TagData[]> = ref([]);

onMounted(() => {
    console.log("mounted");
    loadTags();
});

const loadTags = () => {
    axios.get('/api/tags')
    .then(response => {tags.value = response.data});
}

const deleteTag = (tag:string) => {
    axios.delete('/api/tags/' + tag)
         .then(response => {
            console.log(response);
            loadTags();
         });
}
</script>

<template>
    <table>
        <tr>
            <th>Tag Name</th>
            <th>ID</th>
            <th>Content</th>
            <th>Last Seen</th>
            <th>Actions</th>
        </tr>
        <tr v-for="tag in tags" :item="tag">
            <td>{{ tag.name }}</td>
            <td>{{ tag.tag }}</td>
            <td>{{ tag.contentRef }}</td>
            <td>{{ tag.lastSeen ? formatTimestamp(tag.lastSeen) : "never" }}</td>
            <td><button @click="deleteTag(tag.tag)">Delete</button></td>
        </tr>
    </table>
</template>

<style scoped>
table {
    border-collapse: collapse;
}
td, th {
  border: 1px solid #ddd;
  padding: 8px;
}
th {
  padding-top: 12px;
  padding-bottom: 12px;
  text-align: left;
  background-color: #3670b3;
  color: white;
}
tr:nth-child(even){background-color: #f2f2f2;}
</style>