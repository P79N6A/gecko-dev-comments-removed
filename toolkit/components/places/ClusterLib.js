

















function HierarchicalClustering(distance, merge, threshold) {
  this.distance = distance || clusterlib.euclidean_distance;
  this.merge = merge || clusterlib.average_linkage;
  this.threshold = threshold == undefined ? Infinity : threshold;
}

HierarchicalClustering.prototype = {
  

















  cluster: function HC_cluster(items, snapshotGap, snapshotCallback) {
    
    let clusters = [];
    
    let distances = [];
    
    let neighbors = [];
    
    let clustersByKey = [];

    
    for (let index = 0; index < items.length; index++) {
      let cluster = {
        
        item: items[index],
        
        key: index,
        
        index: index,
        
        size: 1
      };
      clusters[index] = cluster;
      clustersByKey[index] = cluster;
      distances[index] = [];
      neighbors[index] = 0;
    }

    
    for (let i = 0; i < clusters.length; i++) {
      for (let j = 0; j <= i; j++) {
        var dist = (i == j) ? Infinity :
          this.distance(clusters[i].item, clusters[j].item);
        distances[i][j] = dist;
        distances[j][i] = dist;

        if (dist < distances[i][neighbors[i]]) {
          neighbors[i] = j;
        }
      }
    }

    
    let next = null, i = 0;
    for (; next = this.closestClusters(clusters, distances, neighbors); i++) {
      if (snapshotCallback && (i % snapshotGap) == 0) {
        snapshotCallback(clusters);
      }
      this.mergeClusters(clusters, distances, neighbors, clustersByKey,
                         clustersByKey[next[0]], clustersByKey[next[1]]);
    }
    return clusters;
  },

  
















  mergeClusters: function HC_mergeClus(clusters, distances, neighbors,
                                       clustersByKey, cluster1, cluster2) {
    let merged = { item: this.merge(cluster1.item, cluster2.item),
                   left: cluster1,
                   right: cluster2,
                   key: cluster1.key,
                   size: cluster1.size + cluster2.size };

    clusters[cluster1.index] = merged;
    clusters.splice(cluster2.index, 1);
    clustersByKey[cluster1.key] = merged;

    
    for (let i = 0; i < clusters.length; i++) {
      var ci = clusters[i];
      var dist;
      if (cluster1.key == ci.key) {
        dist = Infinity;
      } else if (this.merge == clusterlib.single_linkage) {
        dist = distances[cluster1.key][ci.key];
        if (distances[cluster1.key][ci.key] >
            distances[cluster2.key][ci.key]) {
          dist = distances[cluster2.key][ci.key];
        }
      } else if (this.merge == clusterlib.complete_linkage) {
        dist = distances[cluster1.key][ci.key];
        if (distances[cluster1.key][ci.key] <
            distances[cluster2.key][ci.key]) {
          dist = distances[cluster2.key][ci.key];
        }
      } else if (this.merge == clusterlib.average_linkage) {
        dist = (distances[cluster1.key][ci.key] * cluster1.size
              + distances[cluster2.key][ci.key] * cluster2.size)
              / (cluster1.size + cluster2.size);
      } else {
        dist = this.distance(ci.item, cluster1.item);
      }

      distances[cluster1.key][ci.key] = distances[ci.key][cluster1.key]
                                      = dist;
    }

    
    for (let i = 0; i < clusters.length; i++) {
      var key1 = clusters[i].key;
      if (neighbors[key1] == cluster1.key ||
          neighbors[key1] == cluster2.key) {
        let minKey = key1;
        for (let j = 0; j < clusters.length; j++) {
          var key2 = clusters[j].key;
          if (distances[key1][key2] < distances[key1][minKey]) {
            minKey = key2;
          }
        }
        neighbors[key1] = minKey;
      }
      clusters[i].index = i;
    }
  },

  














  closestClusters: function HC_closestClus(clusters, distances, neighbors) {
    let minKey = 0, minDist = Infinity;
    for (let i = 0; i < clusters.length; i++) {
      var key = clusters[i].key;
      if (distances[key][neighbors[key]] < minDist) {
        minKey = key;
        minDist = distances[key][neighbors[key]];
      }
    }
    if (minDist < this.threshold) {
      return [minKey, neighbors[minKey]];
    }
    return null;
  }
};

let clusterlib = {
  hcluster: function hcluster(items, distance, merge, threshold, snapshotGap,
                              snapshotCallback) {
    return (new HierarchicalClustering(distance, merge, threshold))
           .cluster(items, snapshotGap, snapshotCallback);
  },

  single_linkage: function single_linkage(cluster1, cluster2) {
    return cluster1;
  },

  complete_linkage: function complete_linkage(cluster1, cluster2) {
    return cluster1;
  },

  average_linkage: function average_linkage(cluster1, cluster2) {
    return cluster1;
  },

  euclidean_distance: function euclidean_distance(v1, v2) {
    let total = 0;
    for (let i = 0; i < v1.length; i++) {
      total += Math.pow(v2[i] - v1[i], 2);
    }
    return Math.sqrt(total);
  },

  manhattan_distance: function manhattan_distance(v1, v2) {
    let total = 0;
    for (let i = 0; i < v1.length; i++) {
      total += Math.abs(v2[i] - v1[i]);
    }
    return total;
  },

  max_distance: function max_distance(v1, v2) {
    let max = 0;
    for (let i = 0; i < v1.length; i++) {
      max = Math.max(max, Math.abs(v2[i] - v1[i]));
    }
    return max;
  }
};
