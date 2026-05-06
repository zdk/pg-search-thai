# The "thai" analyzer is built into core Elasticsearch (no plugin install needed).
# Keeping a thin Dockerfile so we have a place to pin the version and add config later.
FROM docker.elastic.co/elasticsearch/elasticsearch:8.15.0
