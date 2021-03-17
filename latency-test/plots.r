library(dplyr)
library(purrr)
library(ggplot2)

reads <- seq(1, 8)

df <- purrr::map_dfr(reads, function(n) {
  file <- paste("read-", n, ".csv", sep="")
  read.csv(file) %>%
    mutate(read_no = n)
})

df.hist <- df %>%
  filter(read_no < 8) %>%
  mutate(
    read_no = factor(read_no),
    latency_group = round(diff_us / 10000)
  ) %>%
  group_by(read_no, latency_group) %>%
  summarize(latency_count = n()) %>%
  mutate(latency_ms = latency_group * 10)

ggplot(df.hist, aes(x = latency_ms, y = latency_count, color = read_no)) +
  geom_point(shape = 3) +
  geom_smooth() +
  scale_x_continuous(breaks = seq(100, 2000, 100))

##  ggsave("latency-hist.pdf")
