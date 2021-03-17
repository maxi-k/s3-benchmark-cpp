library(dplyr)
library(purrr)
library(ggplot2)

reads <- seq(1, 8)
sizes <- c(16, 128, 257)

df <- purrr::map_dfr(reads, function(n) {
  purrr::map_dfr(sizes, function(size) {
    file <- paste(size, "mb/", "read-", n, ".csv", sep="")
    read.csv(file) %>% mutate(read_no = n, size_mb = size)
  })
})

df.hist <- df %>%
  mutate(
    read_f = factor(read_no),
    size_f = factor(size_mb),
    latency_group = diff_us / 1000
  )

ggplot(df.hist, aes(x = latency_group)) +
  geom_histogram() +
  facet_grid(rows = vars(read_f), cols = vars(size_f))
##  ggsave("latency-hist.pdf")

bw <- df.hist %>% group_by(read_f, size_f) %>%
  summarize(bw = (sum(read_byte) / 1024 / 1024) / (overall_us[1] / 1000 / 1000))

min_lat <- df.hist %>%
  group_by(size_mb) %>%
  summarize(
    min_lat = min(latency_group),
    ) %>% ## model: time = 10ms seek + (size / 100mb/s) read
  mutate(model = 10 + (size_mb / 100) * 1000)

ggplot(min_lat, aes(x = model, y = min_lat, color = factor(size_mb))) +
  geom_point() +
  geom_abline() +
  geom_text(aes(label = min_lat), nudge_y = 0.02 * max(min_lat$min_lat))
