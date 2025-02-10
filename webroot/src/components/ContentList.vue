<script setup lang="ts">
import ContentItemComponent from './ContentItemComponent.vue';
import { type ContentItem } from '@/types/ContentItem';
import axios from 'axios';
import { ref, computed } from 'vue';

const items = ref<ContentItem[]>([]);
const filterType = ref<string>('');

const fetchContentList = () => {
    axios.get<ContentItem[]>('/api/content/list')
        .then(response => { items.value = response.data });
};

const reloadContent = () => {
    axios.post('/api/content/reload')
        .then(() => {
            fetchContentList();
        });
};

const filteredItems = computed(() => {
    if (!filterType.value) {
        return items.value;
    }
    return items.value.filter(item => item.type === filterType.value);
});

fetchContentList();
</script>

<template>
    <div class="filter-section">
        <label for="filter-type">Filter by type:</label>
        <select id="filter-type" v-model="filterType">
            <option value="">All</option>
            <option value="music">Music</option>
            <option value="hoerspiel">Hoerspiel</option>
            <!-- Add more options as needed -->
        </select>
    </div>
    <div class="content-elements">        
        <ContentItemComponent v-for="item in filteredItems" :key="item.id" :item="item" />
    </div>
    <button @click="reloadContent" class="reload-button">
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
            <path d="M12 4V1L8 5l4 4V6c3.31 0 6 2.69 6 6s-2.69 6-6 6-6-2.69-6-6H4c0 4.42 3.58 8 8 8s8-3.58 8-8-3.58-8-8-8z"/>
        </svg>
        Reload
    </button>
</template>

<style scoped>
.filter-section {
    margin-bottom: 20px;
}

.reload-button {
    display: flex;
    align-items: center;
    background-color: #007BFF;
    color: white;
    border: none;
    padding: 10px 20px;
    border-radius: 5px;
    cursor: pointer;
    font-size: 16px;
}

.reload-button svg {
    margin-right: 8px;
}

.reload-button:hover {
    background-color: #0056b3;
}

.content-elements {
    display: flex;
    flex-wrap: wrap;
    gap: 1em;
    padding: 5px 0px;
    width: 100%;
}
</style>